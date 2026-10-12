# Target Baseline Evidence Report: Samsung Galaxy Tab A9+ 5G (`SM-X216B`)

**Document Version:** 1.0  
**Target Subject:** Samsung Galaxy Tab A9+ 5G (`SM-X216B` / `gta9p`)  
**Firmware:** `X216BXXS9DYJ7`  
**Binary Analyzed:** `vmlinux.elf` (SHA-256 verified against stock kernel)  
**Primary Target Evidence Sources:**
- `smx216b_mcast_geometry_report.md` [VERIFIED FROM DISASSEMBLY]
- `kernel_profile_verified.json` [VERIFIED FROM DISASSEMBLY + KALLSYMS]
- `target.h` [VERIFIED FROM SOURCE & HEADERS]

---

## 1. Kernel Identity & Environment

| Field | Value | Evidence / Verification Status |
|---|---|---|
| **Device Model** | `SM-X216B` (Samsung Galaxy Tab A9+ 5G) | `kernel_profile_verified.json` [VERIFIED FROM SOURCE] |
| **Codename** | `gta9p` | `kernel_profile_verified.json` [VERIFIED FROM SOURCE] |
| **Firmware Build** | `X216BXXS9DYJ7` | `kernel_profile_verified.json` [VERIFIED FROM SOURCE] |
| **Kernel Release** | `5.4.249-qgki-31053672-abX216BXXS9DYJ7` | `kernel_profile_verified.json` [VERIFIED FROM SOURCE] |
| **Kernel Build Info** | `#1 SMP PREEMPT Wed Oct 15 23:51:00 KST 2025` | `kernel_profile_verified.json` [VERIFIED FROM SOURCE] |
| **Target SoC** | Qualcomm Snapdragon 695 5G (`SM6375`) | `kernel_profile_verified.json` [VERIFIED FROM SOURCE] |
| **Architecture** | AArch64 (ARM64 Little-Endian) | `vmlinux.elf` ELF header [VERIFIED FROM DISASSEMBLY] |
| **Compiler** | Android Clang 11.0.2 (r383902b1), LLD 11.0.2 | `kernel_profile_verified.json` [VERIFIED FROM SOURCE] |
| **Link-Time Base (`_text`)** | `0xffffffc010080000` (`text_offset = 0x80000`) | `vmlinux.elf` ELF headers [VERIFIED FROM DISASSEMBLY] |
| **Kernel Flavor** | Qualcomm QGKI (`CONFIG_QGKI_BUILD=y`) | `kernel_profile_verified.json` [VERIFIED FROM SOURCE] |
| **Security Hardening** | `CONFIG_CFI_CLANG=y`, `CONFIG_SHADOW_CALL_STACK=y`, `CONFIG_ARM64_PAN=y`, `CONFIG_ARM64_UAO=y`, `CONFIG_HARDENED_USERCOPY=y`, `CONFIG_SECURITY_DEFEX=y` | `kernel_profile_verified.json` [VERIFIED FROM SOURCE] |

---

## 2. Verified Kernel Symbol Table

All symbol addresses represent runtime virtual addresses assuming zero KASLR slide (`kaslr_slide = 0`). Symbol file offsets and sizes are verified via `vmlinux.elf` ELF symbol tables and cross-checked against `kallsyms.txt`:

| Symbol Name | Virtual Address (`addr`) | Symbol Offset (`addr - _text`) | Size (Bytes) | Evidence / Status |
|---|---|---|---|---|
| `remove_waiter` | `0xffffffc010376418` | `0x002f6418` | 596 | `vmlinux.elf` symbol table [VERIFIED FROM DISASSEMBLY] |
| `rt_mutex_adjust_prio_chain` | `0xffffffc0103753bc` | `0x002f53bc` | 2004 | `vmlinux.elf` symbol table [VERIFIED FROM DISASSEMBLY] |
| `rt_mutex_start_proxy_lock` | `0xffffffc010376380` | `0x002f6380` | 152 | `vmlinux.elf` symbol table [VERIFIED FROM DISASSEMBLY] |
| `__rt_mutex_start_proxy_lock`| `0xffffffc010375df8` | `0x002f5df8` | 112 | `vmlinux.elf` symbol table [VERIFIED FROM DISASSEMBLY] |
| `rt_mutex_cleanup_proxy_lock`| `0xffffffc010376728` | `0x002f6728` | 156 | `vmlinux.elf` symbol table [VERIFIED FROM DISASSEMBLY] |
| `task_blocks_on_rt_mutex` | `0xffffffc01037606c` | `0x002f606c` | 788 | `vmlinux.elf` symbol table [VERIFIED FROM DISASSEMBLY] |
| `do_futex` | `0xffffffc0103c02dc` | `0x003402dc` | 3544 | `vmlinux.elf` symbol table [VERIFIED FROM DISASSEMBLY] |
| `__arm64_sys_futex` | `0xffffffc0103c3498` | `0x00343498` | 364 | `vmlinux.elf` symbol table [VERIFIED FROM DISASSEMBLY] |
| `futex_requeue` | `0xffffffc0103c162c` | `0x0034162c` | 4108 | `vmlinux.elf` symbol table [VERIFIED FROM DISASSEMBLY] |
| `futex_wait_requeue_pi` | `0xffffffc0103c0a60` | `0x00340a60` | (inlined) | Inlined inside `do_futex` [VERIFIED FROM DISASSEMBLY] |
| `futex_lock_pi` | `0xffffffc0103c2638` | `0x00342638` | 2048 | `vmlinux.elf` symbol table [VERIFIED FROM DISASSEMBLY] |
| `futex_unlock_pi` | `0xffffffc0103c2e38` | `0x00342e38` | 1632 | `vmlinux.elf` symbol table [VERIFIED FROM DISASSEMBLY] |
| `__arm64_sys_setsockopt` | `0xffffffc0112ebbd0` | `0x0126bbd0` | — | `vmlinux.elf` disassembly [VERIFIED FROM DISASSEMBLY] |
| `__sys_setsockopt` | `0xffffffc0112f2f74` | `0x01272f74` | — | `vmlinux.elf` disassembly [VERIFIED FROM DISASSEMBLY] |
| `sock_common_setsockopt` | `0xffffffc0112f95d0` | `0x012795d0` | — | `vmlinux.elf` disassembly [VERIFIED FROM DISASSEMBLY] |
| `ip_setsockopt` | `0xffffffc0113b5cf4` | `0x01335cf4` | — | `vmlinux.elf` disassembly [VERIFIED FROM DISASSEMBLY] |
| `do_ip_setsockopt` | `0xffffffc0113b5d88` | `0x01335d88` | — | `vmlinux.elf` disassembly [VERIFIED FROM DISASSEMBLY] |
| `compat_mc_setsockopt` | `0xffffffc01136292c` | `0x012e292c` | — | `vmlinux.elf` disassembly [VERIFIED FROM DISASSEMBLY] |
| `init_task` | `0xffffffc0129e0280` | `0x02960280` | — | `target.h` / `vmlinux.elf` [VERIFIED FROM DISASSEMBLY] |
| `system_unbound_wq` | `0xffffffc01296f9c8` | `0x028ef9c8` | — | `target.h` / `vmlinux.elf` [VERIFIED FROM DISASSEMBLY] |
| `call_usermodehelper_exec_work` | `0xffffffc010317274` | `0x00297274` | — | Direct function body [VERIFIED FROM DISASSEMBLY] |
| `call_usermodehelper_exec_work` (`.cfi_jt`) | `0xffffffc011681870` | `0x01601870` | 8 | Clang forward-edge CFI jump table thunk [VERIFIED FROM DISASSEMBLY] |
| `anon_pipe_buf_ops` | `0xffffffc0121f55b8` | `0x021755b8` | — | `target.h` / `vmlinux.elf` [VERIFIED FROM DISASSEMBLY] |
| `selinux_state` | `0xffffffc012b9f000` | `0x02b1f000` | — | `target.h` / `vmlinux.elf` [VERIFIED FROM DISASSEMBLY] |

---

## 3. Target `struct rt_mutex_waiter` Structure Layout (Linux 5.4 ABI)

The Linux 5.4 AArch64 ABI defines an **80-byte (`0x50`)** `struct rt_mutex_waiter`.  
This is critically different from the 88-byte (`0x58`) layout used in Linux 5.15+ and Linux 6.1:

| QWORD Index | Byte Offset | Type | Member Name | Size | Exploitation Function / Role |
| :---: | :---: | :--- | :--- | :--- | :--- |
| **Word 0** | `+0x00` | `unsigned long` | `tree_entry.__rb_parent_color` | 8 | RB-tree root / parent color pointer |
| **Word 1** | `+0x08` | `struct rb_node *` | `tree_entry.rb_right` | 8 | Right child pointer in wait_lock tree |
| **Word 2** | `+0x10` | `struct rb_node *` | `tree_entry.rb_left` | 8 | Left child pointer in wait_lock tree |
| **Word 3** | `+0x18` | `unsigned long` | `pi_tree_entry.__rb_parent_color` | 8 | RB-tree root / parent color in task PI tree |
| **Word 4** | `+0x20` | `struct rb_node *` | `pi_tree_entry.rb_right` | 8 | Right child pointer in task PI tree |
| **Word 5** | `+0x28` | `struct rb_node *` | `pi_tree_entry.rb_left` | 8 | Left child pointer in task PI tree |
| **Word 6** | **`+0x30`** | `struct task_struct *` | **`task`** | 8 | Owning/waiting task pointer (dereferenced in PI walk) |
| **Word 7** | **`+0x38`** | `struct rt_mutex *` | **`lock`** | 8 | Target `rt_mutex` pointer (redirected to fake lock) |
| **Word 8** | `+0x40` | `int` + `unsigned short` | `prio` + `deadlocked` | 8 | Waiter priority (`prio` at `+0x40`, 4B; `deadlocked` at `+0x44`, 2B; 2B pad) |
| **Word 9** | `+0x48` | `u64` | `deadline` | 8 | Real-time deadline timestamp |

### Key Structural Differences from Linux 5.15+:
1. `wake_state` (introduced at `+0x40` in 5.15) **does not exist** in Linux 5.4.
2. `ww_ctx` (introduced at `+0x50` in 5.15) **does not exist** in Linux 5.4.
3. Total structure size is strictly **80 bytes (0x50)**, not 88 bytes (`0x58`).
4. In `rt_mutex_adjust_prio_chain()`, the priority chain validation only dereferences `waiter->task` (`+0x30`) and `waiter->lock` (`+0x38`). All active steering words strictly satisfy `waiter_word <= 7`.

---

## 4. Stack Geometry Verification: Futex Frame vs 64-bit MCAST Frame

Disassembly analysis of `/root/s/vmlinux.elf` establishes the mathematical stack coordinates relative to system call entry stack pointer `SP_sys`:

### 4.1 Futex Call Chain (`FUTEX_WAIT_REQUEUE_PI`)

```
el0_svc -> el0_svc_handler (SP_sys - 0x10)
  -> el0_svc_common (SP_sys - 0x50)
    -> __arm64_sys_futex (SP_sys - 0xc0)   [frame: 0x70]
      -> do_futex (SP_sys - 0x120 - 0x1b0 = SP_sys - 0x2d0) [frame: stp x29,x30,[sp,#-96]! + sub sp,sp,#0x1b0]
```

- In Linux 5.4 SM-X216B, `futex_wait_requeue_pi()` is **completely inlined** inside `do_futex()`.
- Waiter frame pointer anchor: `x29 = SP_sys - 0x120`.
- Waiter local allocation: `sub x28, x29, #0xc8`.
- Waiter start address: `(SP_sys - 0x120) - 0xc8 = SP_sys - 0x1e8`.
- Waiter end address (80 bytes): `(SP_sys - 0x1e8) + 0x50 = SP_sys - 0x198`.
- Target fields:
  - `waiter->task` (`+0x30`): `SP_sys - 0x1e8 + 0x30 = SP_sys - 0x1b8`
  - `waiter->lock` (`+0x38`): `SP_sys - 0x1e8 + 0x38 = SP_sys - 0x1b0`

### 4.2 Native 64-bit IPv4 MCAST Call Chain (`MCAST_BLOCK_SOURCE` / `MCAST_JOIN_SOURCE_GROUP`)

```
el0_svc -> el0_svc_handler (SP_sys - 0x10)
  -> el0_svc_common (SP_sys - 0x50)
    -> __arm64_sys_setsockopt (SP_sys - 0x60)     [frame: 0x10]
      -> __sys_setsockopt (SP_sys - 0xd0)         [frame: 0x70]
        -> sock_common_setsockopt (SP_sys - 0x110) [frame: 0x40]
          -> ip_setsockopt (SP_sys - 0x140)       [frame: 0x30]
            -> do_ip_setsockopt (SP_sys - 0x2d0)   [frame: 0x190]
```

- Local buffer: `struct group_source_req` (264 bytes / `0x108`).
- Inside `do_ip_setsockopt()`, destination pointer is passed to `copy_from_user()` via `add x0, sp, #0x20`.
- Buffer start address: `(SP_sys - 0x2d0) + 0x20 = SP_sys - 0x2b0`.
- Buffer end address: `(SP_sys - 0x2b0) + 0x108 = SP_sys - 0x1a8`.

### 4.3 Mathematical Overlap Coordinates

$$\text{Offset of Waiter Base in Buffer} = (\text{SP}_{sys} - 0x1e8) - (\text{SP}_{sys} - 0x2b0) = 0x2b0 - 0x1e8 = 0xc8 \text{ bytes (200 bytes)}$$

| Field | Waiter Struct Offset | Buffer Offset (`0xc8 + field_off`) | Stack Position | Covered by 264-Byte (`0x108`) Buffer? |
|---|---|---|---|:---:|
| `tree_entry` | `+0x00` | `0xc8` (200) | `SP_sys - 0x1e8` | **Yes** |
| `pi_tree_entry` | `+0x18` | `0xe0` (224) | `SP_sys - 0x1d0` | **Yes** |
| **`task`** | **`+0x30`** | **`0xf8` (248)** | `SP_sys - 0x1b8` | **Yes** |
| **`lock`** | **`+0x38`** | **`0x100` (256)** | `SP_sys - 0x1b0` | **Yes (exact boundary)** |
| `prio` | `+0x40` | `0x108` (264) | `SP_sys - 0x1a8` | End boundary |
| `deadline` | `+0x48` | `0x110` (272) | `SP_sys - 0x1a0` | Beyond buffer (+8B) |

> **Critical Geometric Fact:**  
> The 264-byte `group_source_req` buffer spans `[0x00 .. 0x108]`.  
> `waiter->lock` occupies bytes `[0x100 .. 0x108]`, which terminates **at the exact final byte of the copy buffer**.  
> Because `rt_mutex_adjust_prio_chain()` only dereferences `waiter->lock` (`+0x38`) and `waiter->task` (`+0x30`), all required redirection fields fall completely inside the stamped window.

### 4.4 Invalidation of `MCAST_JOIN_GROUP` (optname 42)
- `MCAST_JOIN_GROUP` copies a `struct group_req`, which is only **136 bytes (`0x88`)** in size.
- Buffer end: `(SP_sys - 0x2b0) + 0x88 = SP_sys - 0x228`.
- Waiter base starts at `SP_sys - 0x1e8`.
- Gap: `(SP_sys - 0x1e8) - (SP_sys - 0x228) = 0x40` (64 bytes).
- **Conclusion:** `MCAST_JOIN_GROUP` lacks sufficient stack depth by exactly 64 bytes and cannot touch the waiter.

---

## 5. ARM32 Compat Stamping Geometry (`exp32`)

When invoked from an ARM32 compat thread (`compat_mc_setsockopt` @ `0xffffffc01136292c`):
- The compat layer copies a 260-byte 32-bit user struct (`0x104`) and converts it to a 264-byte 64-bit struct (`0x108`) by expanding alignment.
- Additional intermediate stack frames (`compat_sock_common_setsockopt`, `compat_ip_setsockopt`, `compat_mc_setsockopt`) deepen the call stack by `0x100` bytes.
- Required compat compensation window: `EXP32_STAMP_OFF = 0x60 .. 0x70` (calibrated value: `0x68`).

---

## 6. Comparison with Samsung Linux 5.15 Profiles (X710, X916B, S918B, S916B)

| Feature / Metric | SM-X216B (Linux 5.4.249) | Samsung 5.15 GKI Profiles (X710, X916B, S918B, S916B) | Technical Impact |
|---|---|---|---|
| **Kernel Generation** | Linux 5.4 (Qualcomm QGKI) | Linux 5.15 (Android GKI 13-8) | Frame allocations and ABI are completely disjoint. |
| **`rt_mutex_waiter` Size** | **80 bytes (`0x50`)** | **88 bytes (`0x58`)** | 5.15 introduced `wake_state` (+0x40) and `ww_ctx` (+0x50). |
| **`futex_wait_requeue_pi`** | **Inlined** inside `do_futex()` | **Separate function frame** (`0x1b0`) | Inlined frame in 5.4 alters relative stack offset of waiter. |
| **MCAST Protocol Used** | **IPv4** (`AF_INET`, `IPPROTO_IP`) | **IPv6** (`AF_INET6`, `IPPROTO_IPV6`) | 5.15 profiles use IPv6 `do_ipv6_setsockopt()`; 5.4 uses IPv4 `do_ip_setsockopt()`. |
| **MCAST Copy Buffer** | 264 bytes (`0x108`) | 264 bytes (`0x108`) | Both copy `group_source_req`, but base offsets differ. |
| **Offset in Buffer** | **`0xc8` (200 bytes)** | **`0x78` (120 bytes)** | Attempting to use 5.15's `0x78` offset on SM-X216B misses `waiter->lock` by 80 bytes. |
| **CFI Dispatch** | Requires `.cfi_jt` jump-table thunks (`CONFIG_CFI_CLANG=y`) | Requires `.cfi_jt` jump-table thunks | Both enforce Clang forward-edge CFI. |

---

## 7. Unresolved Questions & Target-Specific Verification Items

1. **`deadline` Field Truncation:**  
   `waiter->deadline` sits at `buffer + 0x110` (8 bytes beyond the 264-byte buffer). Although `rt_mutex_adjust_prio_chain()` does not check deadline unless priorities match, does uninitialized stack memory at `SP_sys - 0x1a0` ever influence scheduling decisions under high system load?
2. **KASLR Slide Leakage Channel:**  
   Samsung Linux 5.4 on SM6375 disables unprivileged `perf_event_open` and restricts `tracefs`. Is CPU0-pinned prefetch side-channel or a different information disclosure required for automated ASLR defeat?
3. **SELinux State Structure:**  
   `SELINUX_STATE_OFF` is resolved to `0x02b1f000`. Does this kernel use `selinux_state.enforcing` or legacy `selinux_enforcing` global symbol? (Disassembly indicates `selinux_state` struct).
