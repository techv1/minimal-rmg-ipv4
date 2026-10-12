#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/sysmacros.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>

extern int run_exploit(int argc, char **argv);

static void fix_tracefs_perms(const char *dir) {
    chmod(dir, 0777);
    DIR *d = opendir(dir);
    if (!d) return;
    struct dirent *de;
    char path[512];
    while ((de = readdir(d)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        snprintf(path, sizeof(path), "%s/%s", dir, de->d_name);
        struct stat st;
        if (stat(path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                chmod(path, 0777);
                fix_tracefs_perms(path);
            } else {
                chmod(path, 0666);
            }
        }
    }
    closedir(d);
}

static void init_mounts(void) {
    mkdir("/proc", 0755);
    mount("proc", "/proc", "proc", 0, NULL);

    mkdir("/sys", 0755);
    mount("sysfs", "/sys", "sysfs", 0, NULL);

    mkdir("/sys/kernel", 0755);
    mkdir("/sys/kernel/tracing", 0755);
    mount("tracefs", "/sys/kernel/tracing", "tracefs", 0, "gid=2000,mode=0777");
    fix_tracefs_perms("/sys/kernel/tracing");

    mkdir("/sys/kernel/debug", 0755);
    mount("debugfs", "/sys/kernel/debug", "debugfs", 0, NULL);

    mkdir("/dev", 0755);
    mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);

    mkdir("/data", 0755);
    mount("tmpfs", "/data", "tmpfs", 0, NULL);
    mkdir("/data/local", 0755);
    mkdir("/data/local/tmp", 0777);

    mknod("/dev/null", S_IFCHR | 0666, makedev(1, 3));
    mknod("/dev/zero", S_IFCHR | 0666, makedev(1, 5));
    mknod("/dev/ptmx", S_IFCHR | 0666, makedev(5, 2));

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

    /* Disable Yama ptrace restrictions so /proc/<child>/mem can be read */
    FILE *yp = fopen("/proc/sys/kernel/yama/ptrace_scope", "w");
    if (yp) {
        fputs("0\n", yp);
        fclose(yp);
    }
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

    if (getpid() == 1) {
        printf("========================================================\n");
        printf("[*] SM-X216B QEMU Init Host (PID 1) Booting...\n");
        printf("========================================================\n");
        init_mounts();

        FILE *cmd = fopen("/proc/cmdline", "r");
        if (cmd) {
            char line[1024];
            if (fgets(line, sizeof(line), cmd) && strstr(line, "nokaslr")) {
                printf("[*] nokaslr detected on cmdline; setting SLIDE_P0_OFFSET=0x0\n");
                setenv("SLIDE_P0_OFFSET", "0x0", 1);
            }
            fclose(cmd);
        }

        pid_t worker = fork();
        if (worker == 0) {
            /* Drop privileges to Android shell user (UID 2000, GID 2000) */
            if (setresgid(2000, 2000, 2000) != 0) {
                perror("setresgid");
            }
            if (setresuid(2000, 2000, 2000) != 0) {
                perror("setresuid");
            }
            prctl(PR_SET_DUMPABLE, 1);
            prctl(0x59616d61, (unsigned long)-1);
            printf("[*] Worker spawned with PID=%d UID=%d EUID=%d GID=%d\n",
                   getpid(), getuid(), geteuid(), getgid());
            int ret = run_exploit(argc, argv);
            printf("[*] Exploit returned: %d\n", ret);
            _exit(ret);
        }

        /* PID 1 host loop: reap children and prevent init panic */
        while (1) {
            int status = 0;
            pid_t waited = waitpid(-1, &status, 0);
            if (waited == worker) {
                printf("[INIT] Worker process %d finished with exit status: %d\n",
                       waited, WIFEXITED(status) ? WEXITSTATUS(status) : status);
                while (1) sleep(100);
            }
        }
    }

    return run_exploit(argc, argv);
}
