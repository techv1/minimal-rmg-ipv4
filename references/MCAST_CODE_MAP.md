# MCAST Code Map & Architectural Flow Analysis

**Document Version:** 1.0  
**Focus:** Code-level comparative mapping of IPv4/IPv6 Multicast stack-writer mechanisms across GhostLock implementations  
**Scope:** Source-code and disassembly analysis across 8 reference repositories and target `SM-X216B`  

---

## 1. Architectural Concept: Why Multicast Setsockopt Serves as a Stack Carrier

In CVE-2026-43499 (GhostLock), a race condition during `FUTEX_WAIT_REQUEUE_PI` leaves a dangling pointer in `task->pi_blocked_on` pointing to an unlinked `struct rt_mutex_waiter` allocated on the waiter thread's kernel stack.

When the waiter thread returns to userspace, its kernel stack unwinds. To control subsequent priority-inheritance walks (`rt_mutex_adjust_prio_chain`), the thread immediately executes a system call that copies attacker-controlled bytes onto its own kernel stack, landing directly over the memory location previously occupied by the waiter struct.

### Why the Multicast API is Unique:
1. **Large Single-Call Copy Size:** Options utilizing `struct group_source_req` (`MCAST_BLOCK_SOURCE`, `MCAST_JOIN_SOURCE_GROUP`) copy **264 bytes (`0x108`)** in a single contiguous `copy_from_user` / `copy_from_sockptr`.
2. **Deep Stack Frame:** Network protocol option processing traverses socket abstraction layers (`sys_setsockopt` → `sock_common_setsockopt` → `ip_setsockopt` → `do_ip_setsockopt`), creating an accumulated frame depth that reaches down to the futex allocation depth.
3. **Clean Error Early-Exit:** By supplying an unconfigured multicast address or explicitly zeroing `gsr_group.ss_family`, the kernel executes `copy_from_user` to the local stack variable first, and subsequently encounters an address/family check failure, cleanly returning `-EADDRNOTAVAIL` or `-EINVAL` without altering real network multicast group membership or kernel heap state.

---

## 2. Global Call Chain Comparison: Futex vs MCAST

```
========================================================================================================
FUTEX ALLOCATION CHAIN                                  MCAST STACK-COPY CHAIN (IPv4)
========================================================================================================
[el0_svc]                                               [el0_svc]
   │                                                       │
   ▼                                                       ▼
[el0_svc_handler]                                       [el0_svc_handler]
   │                                                       │
   ▼                                                       ▼
[el0_svc_common]                                        [el0_svc_common]
   │                                                       │
   ▼                                                       ▼
[__arm64_sys_futex]                                     [__arm64_sys_setsockopt]
   │                                                       │
   ▼                                                       ▼
[do_futex]                                              [__sys_setsockopt]
   │                                                       │
   ├─ (Linux 5.4: inlined)                                 ▼
   │  Local: struct rt_mutex_waiter                     [sock_common_setsockopt]
   │                                                       │
   └─ (Linux 5.15 / 6.1: function call)                    ▼
         ▼                                              [ip_setsockopt]
      [futex_wait_requeue_pi]                              │
         Local: struct rt_mutex_waiter                     ▼
                                                        [do_ip_setsockopt]
                                                           Local: struct group_source_req (264 bytes)
                                                           Operation: copy_from_user(sp + 0x20, user, 0x108)
========================================================================================================
```

---

## 3. Project-by-Project Code & Function Map

### 3.1 Samsung Galaxy Tab A9+ 5G (`SM-X216B` / Target Baseline)

```
[Userspace] setsockopt(fd, IPPROTO_IP, MCAST_BLOCK_SOURCE, &gsr, 264)
    │
    ▼
[Kernel Entry: el0_svc_common]
    │   SP = SP_sys - 0x50
    ▼
[__arm64_sys_setsockopt @ 0xffffffc0112ebbd0] (Frame: 0x10)
    │   SP = SP_sys - 0x60
    ▼
[__sys_setsockopt @ 0xffffffc0112f2f74] (Frame: 0x70)
    │   SP = SP_sys - 0xd0
    ▼
[sock_common_setsockopt @ 0xffffffc0112f95d0] (Frame: 0x40)
    │   SP = SP_sys - 0x110
    ▼
[ip_setsockopt @ 0xffffffc0113b5cf4] (Frame: 0x30)
    │   SP = SP_sys - 0x140
    ▼
[do_ip_setsockopt @ 0xffffffc0113b5d88] (Frame: 0x190)
        SP = SP_sys - 0x2d0
        Buffer Destination: sp + 0x20 = SP_sys - 0x2b0
        Copy: copy_from_user(sp + 0x20, optval, 264)
```

| Source / Binary File | Function / Section | Role in Chain | MCAST Opt & Struct | Stack Frame Depth | Confidence Level |
|---|---|---|---|---|---|
| `vmlinux.elf` | `do_ip_setsockopt` (`0xffffffc0113b5d88`) | IPv4 socket option handler | `MCAST_BLOCK_SOURCE` (43) / `group_source_req` | Frame `0x190`, buffer `sp+0x20` | `VERIFIED FROM DISASSEMBLY` |
| `vmlinux.elf` | `compat_mc_setsockopt` (`0xffffffc01136292c`) | 32-bit compat socket handler | `group_source_req32` (260B -> 264B) | Frame expansion + `0x100` | `VERIFIED FROM DISASSEMBLY` |
| `target.h` | Lines 42-57, 75-85 | Header configuration | `MCAST_64_WAITER_BASE_OFF = 0xc8` | `MCAST_BUFFER_SIZE_64 = 264` | `VERIFIED FROM SOURCE` |

---

### 3.2 BuSung-dev/Root-My-Galaxy-Payloads (Samsung 5.15 Kalama Family: X710, X916B, S918B, S916B)

```
[Userspace] setsockopt(fd, IPPROTO_IPV6, MCAST_JOIN_SOURCE_GROUP, &gsr, 264)
    │
    ▼
[invoke_syscall] (Syscall entry accum: 0x60)
    │   SP = E - 0x60
    ▼
[__arm64_sys_setsockopt] (Frame: 0x10)
    │   SP = E - 0x70
    ▼
[__sys_setsockopt] (Frame: 0x70)
    │   SP = E - 0xe0
    ▼
[sock_common_setsockopt] (Frame: 0x10)
    │   SP = E - 0xf0
    ▼
[ipv6_setsockopt] (Frame: 0x40)
    │   SP = E - 0x130
    ▼
[do_ipv6_setsockopt] (Frame: 0x2c0)
        SP = E - 0x3f0
        Buffer Destination: sp + 0x40 = E - 0x3b0
        Copy: copy_from_user(sp + 0x40, optval, 264)
```

| Source File | Function / Section | Role in Chain | MCAST Opt & Struct | Stack Frame Depth | Confidence Level |
|---|---|---|---|---|---|
| `docs/SM-X916B-X916BXXS6EZG3.md` | § "MCAST_WAITER_OFF derivation" | Documentation of disassembly | `MCAST_JOIN_SOURCE_GROUP` (IPv6, 46) / `group_source_req` | Buffer at `E - 0x3b0`, Waiter at `E - 0x338` (`MCAST_WAITER_OFF = 0x78`) | `VERIFIED FROM DISASSEMBLY` |
| `src/slide_app.c` | `slide_mcast_stack_copy()` | Runtime stack stamping routine | `setsockopt(AF_INET6, IPPROTO_IPV6, ...)` | Transports fake waiter at buffer offset `0x78` | `VERIFIED FROM SOURCE` |
| `docs/SM-X710-X710XXS6EZF1.md` | § "GhostLock note" | Explains why `pselect` was rejected | `pselect` delta -200B vs MCAST | Geometry infeasibility analysis | `DOCUMENTED BY PROJECT AUTHOR` |

---

### 3.3 newazbenalam/Root-My-Galaxy-Payloads (Samsung S21 FE G990B2 - Linux 5.4 Baseline)

```
[Userspace] pselect(nfds, &in, &out, &ex, timeout, NULL)
    │
    ▼
[core_sys_select]
    │   Copies fd_set words onto stack frame
    ▼
[Stack Stamping Effect]
    Places 80-byte fake waiter onto stack:
    task@0x30, lock@0x38, prio@0x40, deadline@0x48
```

| Source File | Function / Section | Role in Chain | Carrier / Struct | Stack Frame Relevance | Confidence Level |
|---|---|---|---|---|---|
| `src/targets/r9q-G990B2XXSJIZF1/target.h` | Lines 24-25, 131-137 | Target ABI definition | `LEGACY_RT_MUTEX_WAITER 1`, size `0x50` | Proves Samsung 5.4 uses 80B waiter | `VERIFIED FROM SOURCE` |
| `src/fops.c` | `do_pselect_fake_lock_route()` | `pselect` stack writer | `pselect` fd_sets | No MCAST implementation in repo | `VERIFIED FROM SOURCE` |

---

### 3.4 wonjj6768/Lenovo-TB365FC-GhostLock-CVE-2026-43499

```
[Userspace] setsockopt(fd, IPPROTO_IP, MCAST_BLOCK_SOURCE, gsr, 264)
    │
    ▼
[Kernel IPv4 setsockopt chain]
    │
    ▼
[do_ip_setsockopt]
    Dest: sp + local_offset
    Wait offset: STAMP_OFF = 0x60
    Fields: b + 0x60 + 0x30 (task), b + 0x60 + 0x38 (lock)
```

| Source File | Function / Section | Role in Chain | MCAST Opt & Struct | Stack Frame Relevance | Confidence Level |
|---|---|---|---|---|---|
| `src/poc_mcast_root.c` | `mcast_stamp_stack()` (lines 457-487) | Inlined UDP socket stamp | `MCAST_BLOCK_SOURCE` (43) / `group_source_req` (0x108) | `STAMP_OFF = 0x60` | `VERIFIED FROM SOURCE` |
| `src/poc_mcast_root.c` | `thread_Y()` (lines 492-535) | Waiter thread topology | Triggered via `sched_yield` loop | Syscall-free spinning post-stamp | `VERIFIED FROM SOURCE` |

---

### 3.5 AnonymousUser369/GhostLockAdapt (POCO air Linux 5.15.180)

```
[Userspace] setsockopt(fd, IPPROTO_IP, MCAST_BLOCK_SOURCE, buf, 0x108)
    │
    ▼
[__arm64_sys_setsockopt] (0x10)
    │
    ▼
[__sys_setsockopt] (0x80)
    │
    ▼
[sock_common_setsockopt] (0x40)
    │
    ▼
[CFI slowpath] (+0x20)
    │
    ▼
[udp_setsockopt] (0x10)
    │
    ▼
[ip_setsockopt @ 0x408248] (0x290)
        memset(sp+0x18, 0, 0x108)
        copy_from_sockptr(sp+0x18, optval, 0x108)
        greqs = E - 0x378; waiter = E - 0x318
        WAITER_OFF = 0x378 - 0x318 = 0x60
```

| Source File | Function / Section | Role in Chain | MCAST Opt & Struct | Stack Frame Relevance | Confidence Level |
|---|---|---|---|---|---|
| `exploit-finale/docs/WRITE-PRIMITIVES.md` | § 1 ("IPv4 MCAST_BLOCK_SOURCE") | Comprehensive frame disassembly analysis | `MCAST_BLOCK_SOURCE` (43) / `group_source_req` (0x108) | `ip_setsockopt` frame 0x290, `WAITER_OFF = 0x60` | `VERIFIED FROM DISASSEMBLY` |
| `Target/waiter_field_offsets_5-15-180.md` | Disassembly proof | llvm-objdump analysis of `task_blocks_on_rt_mutex` | Compact `rt_mutex_waiter` (88B, 0x58) | `task@0x30`, `lock@0x38`, `wake_state@0x40` | `VERIFIED FROM DISASSEMBLY` |
| `poc-mcast-root/src/poc_mcast_root.c` | `mcast_stamp_stack()` | Experimental PoC stamp routine | `MCAST_BLOCK_SOURCE` (43) | `MCAST_PROBE_INDEX=1` dynamic derivation | `VERIFIED FROM SOURCE` |

---

### 3.6 zzzxxxxxxxxxx/GhostLock-GOT-W29 (Huawei MatePad Pro 11 Linux 4.19.157)

```
[Userspace] setsockopt(fd, SOL_IP, MCAST_BLOCK_SOURCE, buf, 0x108)
    │
    ▼
[__arm64_sys_setsockopt] (0x50)
    │
    ▼
[sock_common_setsockopt] (0x10)
    │
    ▼
[udp_setsockopt] (0x10)
    │
    ▼
[ip_setsockopt] (0x30)
    │
    ▼
[do_ip_setsockopt] (0x190)
        Copy dest: sp + 0x20 = E - 0x210
        Waiter at: E - 0x1b0
        MCAST_WAITER_OFF = (E - 0x1b0) - (E - 0x210) = 0x60
```

| Source File | Function / Section | Role in Chain | MCAST Opt & Struct | Stack Frame Relevance | Confidence Level |
|---|---|---|---|---|---|
| `exploit/ghostlock-source/src/slide.c` | Lines 14-20, 249-261 | MCAST stamp + pselect park | `MCAST_BLOCK_SOURCE` (43) / `group_source_req` (0x108) | `MCAST_WAITER_OFF = 0x60` | `VERIFIED FROM DISASSEMBLY` |
| `exploit/ghostlock-source/src/target.h` | Lines 39-46 | Target layout definition | `rt_mutex_waiter` (11 words, 88B) | `major_prio@0x40`, `major_only@0x44` | `VERIFIED FROM SOURCE` |

---

### 3.7 zhubaohe123/ghostlock-kit (vivo iQOO Neo9S Pro Linux 6.1.145)

```
[Userspace] setsockopt(fd, IPPROTO_IP, MCAST_JOIN_SOURCE_GROUP, buf, 0x108)
    │
    ▼
[__arm64_sys_setsockopt @ 0xda3428] (0x10)
    │
    ▼
[__sys_setsockopt @ 0xda3258] (0x60)
    │
    ▼
[sock_common_setsockopt @ 0xdaedc4] (0x10)
    │
    ▼
[tcp_setsockopt @ 0xee7214] (0x10)
    │
    ▼
[ip_setsockopt @ 0xed8960] (0x40)
    │
    ▼
[do_ip_setsockopt @ 0xed750c] (0x280)
        Frame accum: 0x350
        Destination: sp + 0x18 = S' - 0x338
        Waiter at: S' - 0x2D8
        Buffer Offset: (S' - 0x2D8) - (S' - 0x338) = 0x60
        Validation check: buf[8] == 0 (gsr_group.ss_family) -> b.ne -> -EADDRNOTAVAIL (early return)
```

| Source File | Function / Section | Role in Chain | MCAST Opt & Struct | Stack Frame Relevance | Confidence Level |
|---|---|---|---|---|---|
| `docs/2026-09-13_技术分析-mcast-transport.md` | § 2-4 | Disassembly of setsockopt chain | `MCAST_JOIN_SOURCE_GROUP` (46) / `group_source_req` (0x108) | `do_ip_setsockopt` frame 0x280, destination `sp+0x18`, offset `0x60` | `VERIFIED FROM DISASSEMBLY` |
| `docs/2026-09-13_技术分析-rtmutex-waiter与PI-walk.md` | § 2-3 | Futex chain and waiter disassembly | Compact 6.1 `rt_mutex_waiter` (88B, 0x58) | `futex_wait_requeue_pi` waiter at `sp+0x28`, total `S' - 0x2D8` | `VERIFIED FROM DISASSEMBLY` |
| `src/ghostlock-mcast.patch` | `do_mcast_fake_lock_route()` | Production patch for MCAST | Optname 46, `buf[8] = 0` early return | Live stamping and walk trigger | `VERIFIED FROM SOURCE` |

---

## 4. Generic vs Kernel-Build-Specific Components

| Component | Nature | Portability Across Devices / Kernels |
|---|---|---|
| **Userspace API invocation** (`setsockopt`) | **Generic** | Portable. Calling `setsockopt(fd, IPPROTO_IP, optname, buf, 264)` has uniform userspace signature. |
| **Data structure copied** (`struct group_source_req`) | **Generic** | Portable. The 264-byte (`0x108`) size is standardized by POSIX / Linux UAPI (`in.h`). |
| **Family check early-exit mechanism** (`ss_family == 0`) | **Generic** | Highly portable across Linux 4.19, 5.4, 5.15, 6.1. All kernels test `ss_family == AF_INET` and return `-EADDRNOTAVAIL` if zero. |
| **`struct rt_mutex_waiter` layout** | **Kernel-Generation Specific** | **NOT PORTABLE**. Linux 5.4 uses 80 bytes (`0x50`); Linux 5.15 and 6.1 use 88 bytes (`0x58`, compact with `wake_state`); Huawei 4.19 uses 88 bytes (`CONFIG_HW_FUTEX_PI`). |
| **Syscall entry wrappers & Frame sizes** | **Kernel & Compiler Specific** | **NOT PORTABLE**. Linux 5.4 uses `el0_svc_handler` + `el0_svc_common`; 5.15 uses `do_el0_svc` + `invoke_syscall`. Frame sizes vary with Clang optimization, PGO/BOLT, and CFI slowpaths. |
| **Futex function inlining** | **Kernel-Build Specific** | **NOT PORTABLE**. In Linux 5.4 `SM-X216B`, `futex_wait_requeue_pi` is inlined into `do_futex()`; in 5.15/6.1 it occupies a dedicated 0x1b0/0x1c0 stack frame. |
| **Exact Stamping Offset (`WAITER_OFF`)** | **Device & Kernel Specific** | **NOT PORTABLE**. Varies widely: `0xc8` (SM-X216B 5.4 IPv4), `0x78` (Samsung 5.15 IPv6), `0x60` (GhostLockAdapt 5.15 IPv4, Huawei 4.19 IPv4, iQOO 6.1 IPv4). |
