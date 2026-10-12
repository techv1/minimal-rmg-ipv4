# Comparative Study: MCAST Transports, Waiter Layouts, and Target Distance Analysis

**Document Version:** 1.0  
**Target Subject:** Samsung Galaxy Tab A9+ 5G (`SM-X216B`, `5.4.249-qgki-31053672-abX216BXXS9DYJ7`, Qualcomm SM6375)  
**Compared Implementations:**
1. Huawei MatePad Pro 11 (`GOT-W29`, Linux 4.19)
2. Samsung Galaxy S21 FE (`SM-G990B2`, Linux 5.4)
3. Target Samsung Galaxy Tab A9+ 5G (`SM-X216B`, Linux 5.4)
4. Samsung Galaxy Z Fold4 / A53 (`SM-F9360` / `SM-A536E`, Linux 5.10)
5. Samsung Galaxy Tab S9 / S23 Family (`SM-X710`, `SM-X916B`, `SM-S918B`, `SM-S916B`, Linux 5.15)
6. Lenovo Tab (`TB365FC`, Linux 5.15)
7. POCO air / GhostLockAdapt (`air-AP3A`, Linux 5.15)
8. vivo / iQOO Neo9S Pro (`V2339FA`, Linux 6.1)

---

## 1. Waiter Structure Layout Matrix Across Kernel Generations

The following matrix compares `struct rt_mutex_waiter` across all surveyed kernel generations and implementations. All entries are explicitly backed by disassembly or official target headers:

| Implementation / Kernel | Total Size | `tree_entry` | `pi_tree_entry` | `task` | `lock` | Field at `+0x40` | Field at `+0x44` | Field at `+0x48` | Field at `+0x50` | Evidence Status |
|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **Huawei GOT-W29** (Linux 4.19.157 + `CONFIG_HW_FUTEX_PI`) | **88 B** (`0x58`) | `+0x00` (24B) | `+0x18` (24B) | `+0x30` (8B) | `+0x38` (8B) | `major_prio` (4B) | `major_only` (4B) | `prio` (4B) + 4B pad | `deadline` (8B) | `VERIFIED FROM DISASSEMBLY` |
| **Samsung G990B2** (Linux 5.4.x / Qualcomm SM8350) | **80 B** (`0x50`) | `+0x00` (24B) | `+0x18` (24B) | `+0x30` (8B) | `+0x38` (8B) | `prio` (4B) | `deadlocked` (2B) + 2B pad | `deadline` (8B) | *(does not exist)* | `VERIFIED FROM SOURCE` |
| **Samsung SM-X216B** (Linux 5.4.249 / Qualcomm SM6375) | **80 B** (`0x50`) | `+0x00` (24B) | `+0x18` (24B) | `+0x30` (8B) | `+0x38` (8B) | `prio` (4B) | `deadlocked` (2B) + 2B pad | `deadline` (8B) | *(does not exist)* | `VERIFIED FROM DISASSEMBLY` |
| **Samsung F9360 / A536E** (Linux 5.10.x GKI) | **80 B** (`0x50`) | `+0x00` (24B) | `+0x18` (24B) | `+0x30` (8B) | `+0x38` (8B) | `prio` (4B) | `deadlocked` (2B) + 2B pad | `deadline` (8B) | *(does not exist)* | `VERIFIED FROM SOURCE` |
| **Samsung X710 / X916B / S918B / S916B** (Linux 5.15.189 GKI) | **88 B** (`0x58`) | `+0x00` (24B) | `+0x18` (24B) | `+0x30` (8B) | `+0x38` (8B) | `wake_state` (4B) | `prio` (4B) | `deadline` (8B) | `ww_ctx` (8B) | `VERIFIED FROM DISASSEMBLY` |
| **Lenovo TB365FC** (Linux 5.15.170 GKI) | **88 B** (`0x58`) | `+0x00` (24B) | `+0x18` (24B) | `+0x30` (8B) | `+0x38` (8B) | `wake_state` (4B) | `prio` (4B) | `deadline` (8B) | `ww_ctx` (8B) | `VERIFIED FROM SOURCE` |
| **GhostLockAdapt / POCO air** (Linux 5.15.180 GKI) | **88 B** (`0x58`) | `+0x00` (24B) | `+0x18` (24B) | `+0x30` (8B) | `+0x38` (8B) | `wake_state` (4B) | `prio` (4B) | `deadline` (8B) | `ww_ctx` (8B) | `VERIFIED FROM DISASSEMBLY` |
| **vivo iQOO Neo9S Pro** (Linux 6.1.145 GKI) | **88 B** (`0x58`) | `+0x00` (24B) | `+0x18` (24B) | `+0x30` (8B) | `+0x38` (8B) | `wake_state` (4B) | `prio` (4B) | `deadline` (8B) | `ww_ctx` (8B) | `VERIFIED FROM DISASSEMBLY` |

### Key Structural Findings:
1. **The 80-byte vs 88-byte ABI Divide:**
   - Linux 5.4 (`SM-X216B`, `G990B2`) and Linux 5.10 (`F9360`, `A536E`) use the **80-byte (`0x50`) legacy layout**.
   - Linux 5.15+ introduced `wake_state` at `+0x40` and `ww_ctx` at `+0x50`, expanding the struct to **88 bytes (`0x58`)**.
   - Huawei Linux 4.19 also has 88 bytes, but solely due to proprietary vendor fields (`major_prio` and `major_only`) from `CONFIG_HW_FUTEX_PI`, which are completely absent in vanilla and Android GKI kernels.
2. **Redirection Word Invariance:**
   - In **every single surveyed kernel**, `waiter->task` sits at byte offset **`+0x30`** (Word 6) and `waiter->lock` sits at byte offset **`+0x38`** (Word 7).
   - In `rt_mutex_adjust_prio_chain()`, the loop reads `waiter->lock` and follows `waiter->task`. The active steering words strictly require controlling `waiter_word <= 7`.

---

## 5-Dimensional Reference Distance & Similarity Matrix

The table below evaluates every reference against `SM-X216B` across five critical dimensions:
1. **Kernel Generation Similarity:** Closeness of Linux kernel major/minor version and core locking subsystem ABI.
2. **Samsung / Qualcomm Similarity:** Degree of shared vendor hardening (Samsung Defex, knox/RKP), QGKI patches, and Qualcomm platform architecture.
3. **MCAST Implementation Similarity:** Protocol choice (IPv4 vs IPv6), optname family, copy buffer size, and early-exit mechanism.
4. **Stack-Frame Similarity:** Inlining decisions, entry wrapper layout, and relative offset between MCAST buffer and waiter base.
5. **Usefulness as a Study Reference:** Overall pedagogical value for understanding specific phases of the research pipeline.

| Reference Project | Kernel Generation Similarity | Samsung / Qualcomm Similarity | MCAST Implementation Similarity | Stack-Frame Similarity | Usefulness as a Study Reference |
|---|---|---|---|---|---|
| **Huawei GOT-W29** (`4.19.157-perf+`) | **Low** (Linux 4.19; non-GKI legacy branch) | **Moderate** (Qualcomm SM8250, but HarmonyOS / non-Samsung) | **High** (IPv4 `MCAST_BLOCK_SOURCE` optname 43; 264B copy; `do_ip_setsockopt` frame 0x190) | **Moderate** (`do_ip_setsockopt` buffer starts at `sp+0x20`; but entry wrappers are shallower, giving `WAITER_OFF = 0x60`) | High for studying pre-5.10 IPv4 multicast mechanics and in-kernel thread parking. |
| **Samsung G990B2** (`5.4` / `r9q`) | **Highest** (Exact Linux 5.4 match; identical 80B waiter layout) | **Highest** (Samsung One UI + Qualcomm SM8350 Snapdragon 888) | **None** (Repo uses `pselect6` / `fd_set`; no MCAST implemented) | **High (Futex frame only)** (Shares Qualcomm 5.4 inlined futex frame, but lacks MCAST setsockopt frame) | Essential baseline for validating Samsung Linux 5.4 waiter offsets, KASLR anchors, and UMH escalation structures. |
| **Samsung X710 / X916B / S918B / S916B** (`5.15.189`) | **Low** (Linux 5.15 GKI; 88B compact waiter with `wake_state`) | **High** (Samsung One UI + Qualcomm Snapdragon 8 Gen 2) | **Moderate** (Uses **IPv6** `MCAST_JOIN_SOURCE_GROUP` optname 46; 264B copy in `do_ipv6_setsockopt`) | **Low** (IPv6 chain and 5.15 futex frame result in `MCAST_WAITER_OFF = 0x78` vs target `0xc8`) | High for Samsung-specific post-corruption pipeline (fake ashmem fops, configfs ARW, pipe buffers, UMH). |
| **Lenovo TB365FC** (`5.15.170`) | **Low** (Linux 5.15 GKI; 88B compact waiter) | **Lowest** (Lenovo ZUXOS / MediaTek SoC; no Samsung hardening) | **High** (IPv4 `MCAST_BLOCK_SOURCE` optname 43; 264B copy; UDP socket) | **Low** (`STAMP_OFF = 0x60` due to 5.15 non-inlined futex frame) | High for studying compact standalone PoC structure and DirtyPipe cred patching. |
| **GhostLockAdapt** (`5.15.180`) | **Low** (Linux 5.15 GKI; 88B compact waiter) | **Low** (Xiaomi HyperOS / POCO air) | **Highest** (IPv4 `MCAST_BLOCK_SOURCE` optname 43; 264B copy; explicit proof of IPv6 depth insufficiency) | **Low** (`WAITER_OFF = 0x60` due to 5.15 frame layout and CFI slowpath) | Invaluable for carrier comparison methodology, dynamic probing (`MCAST_PROBE_INDEX`), and disassembly proof. |
| **vivo iQOO Neo9S Pro** (`6.1.145`) | **Lowest** (Linux 6.1; modern 6.x GKI branch) | **Lowest** (vivo OriginOS / MediaTek MT6989; +PGO/+BOLT compiler layout) | **Highest** (IPv4 `MCAST_JOIN_SOURCE_GROUP` optname 46; 264B copy; zeroed `ss_family` early-exit) | **Low** (`WAITER_OFF = 0x60` with deeper vendor frames: `do_ip_setsockopt` 0x280) | Benchmark reference for demonstrating zeroed `ss_family` early exit (`errno = 99`) without multicast side effects. |

---

## 3. Direct Evidence-Backed Analysis: The Core Question

The prompt poses the explicit inquiry:

> **Which of Huawei, G990B2, X710, X916B, S918B, S916B, Lenovo, GhostLockAdapt, and iQOO/Vivo is closest to SM-X216B for:**  
> **(a) kernel generation,**  
> **(b) MCAST implementation, and**  
> **(c) Samsung/Qualcomm stack geometry?**

### (a) Closest Reference for Kernel Generation: **Samsung Galaxy S21 FE (`SM-G990B2`)**
- **Evidence:**
  - `SM-X216B` runs Linux `5.4.249`.
  - `SM-G990B2` (`r9q-G990B2XXSJIZF1`) is the **only reference in the entire survey running Linux 5.4**.
  - All other references run either Linux 4.19 (Huawei), Linux 5.10 (F9360/A536E), Linux 5.15 (X710, X916B, S918B, S916B, Lenovo, GhostLockAdapt), or Linux 6.1 (iQOO).
  - G990B2 explicitly defines `LEGACY_RT_MUTEX_WAITER 1` and `FAKE_WAITER_LAYOUT_SIZE 0x50` (80 bytes), with identical field offsets (`task` at `0x30`, `lock` at `0x38`, `prio` at `0x40`, `deadline` at `0x48`), proving 100% structural parity with SM-X216B's `rt_mutex_waiter` ABI.

### (b) Closest Reference for MCAST Implementation: **GhostLockAdapt (`AnonymousUser369`) & iQOO Neo9S Pro (`zhubaohe123`)**
- **Evidence:**
  - `SM-X216B` utilizes native **64-bit IPv4** multicast socket options (`MCAST_BLOCK_SOURCE` optname 43 or `MCAST_JOIN_SOURCE_GROUP` optname 46), copying `sizeof(struct group_source_req) = 264` bytes (`0x108`) to `do_ip_setsockopt()`, with family-check early exit.
  - **Samsung 5.15 references (`BuSung-dev`: X710, X916B, S918B)** did **not** use IPv4 multicast; they used **IPv6** multicast (`AF_INET6`, `IPPROTO_IPV6`, `MCAST_JOIN_SOURCE_GROUP`) through `do_ipv6_setsockopt()`.
  - **Samsung G990B2** did **not** implement MCAST at all (it relied on `pselect6`).
  - **GhostLockAdapt** specifically implemented and proved IPv4 `MCAST_BLOCK_SOURCE` (43) copying 264 bytes via UDP socket, and documented why IPv6 is geometrically invalid on mobile kernels due to excessive call depth.
  - **iQOO Neo9S Pro (`ghostlock-kit`)** implemented IPv4 `MCAST_JOIN_SOURCE_GROUP` (46) copying 264 bytes, and pioneered the technique of explicitly zeroing `buf[8]` (`gsr_group.ss_family = 0`) to trigger `EADDRNOTAVAIL` early exit immediately post-copy.
  - Therefore, GhostLockAdapt and iQOO Neo9S Pro provide the closest functional models of the IPv4 264-byte `group_source_req` carrier mechanism used on SM-X216B.

### (c) Closest Reference for Samsung/Qualcomm Stack Geometry: **No Directly Portable Reference; Structural Alignment with Samsung G990B2 & Huawei GOT-W29**
- **Evidence:**
  - **Samsung 5.15 GKI profiles (X710, X916B, S918B, S916B)** have completely incompatible stack geometry:
    - Their futex chain allocates a separate `futex_wait_requeue_pi()` frame (`0x1b0`), placing the waiter at `E - 0x338`.
    - Their MCAST chain uses IPv6 (`do_ipv6_setsockopt`), placing the buffer at `E - 0x3b0`.
    - Their offset is `MCAST_WAITER_OFF = 0x78` (120 bytes). Attempting to use `0x78` on SM-X216B (which requires `0xc8` = 200 bytes) misses `waiter->lock` by exactly 80 bytes.
  - **Huawei GOT-W29 (Qualcomm Snapdragon 865 / Linux 4.19)** shares the IPv4 `do_ip_setsockopt(0x190)` local buffer placement at `sp + 0x20` (identical to SM-X216B), but its earlier entry wrappers are significantly shallower (`0x50` total vs 5.4's deeper frames), resulting in `WAITER_OFF = 0x60` (96 bytes).
  - **Samsung G990B2 (Qualcomm Snapdragon 888 / Linux 5.4)** shares the exact Qualcomm Linux 5.4 system call wrapper hierarchy (`el0_svc_handler` + `el0_svc_common` + `__arm64_sys_futex`) and inlines `futex_wait_requeue_pi` inside `do_futex()`. However, because G990B2 used `pselect6`, it provides no setsockopt stack coordinates.
  - **Conclusion:** On `SM-X216B`, the mathematical distance from the start of the 264-byte MCAST buffer to the waiter base is **`0xc8` (200 bytes)**, with `waiter->lock` landing at **`0x100` (256 bytes)**. This geometry is **strictly unique to the SM-X216B Linux 5.4 Qualcomm build** and cannot be imported from any external reference.

---

## 4. Synthesis and Porting Implications

```
                     ┌─────────────────────────────────────────────────────────┐
                     │          TARGET UNDER STUDY: SM-X216B                   │
                     │ Linux 5.4.249 · Qualcomm SM6375 · 80-byte waiter ABI   │
                     │ Native IPv4 MCAST: WAITER_BASE = 0xc8, LOCK = 0x100     │
                     └─────────────────────────────────────────────────────────┘
                                                  │
                 ┌────────────────────────────────┼────────────────────────────────┐
                 ▼                                ▼                                ▼
  [ KERNEL GEN / WAITER ABI ]          [ MCAST TRANSPORT MODEL ]         [ POST-CORRUPTION PIPELINE ]
      Closest: Samsung G990B2             Closest: GhostLockAdapt &          Closest: Samsung Kalama
      - 80-byte waiter layout                      iQOO Neo9S Pro                    (X710, X916B, S918B)
      - Legacy prio@0x40                   - IPv4 UDP 264B copy               - Fake ashmem fops
      - No wake_state / ww_ctx             - ss_family = 0 early exit         - Pipe buffer ARW
      - Clang 11 / QGKI base               - Dynamic probe discipline         - Workqueue UMH escalation
```

> [!WARNING]
> **Portability Fallacy:**  
> A reference cannot be considered "portable" simply because it implements an MCAST carrier.  
> As demonstrated above:
> - Borrowing waiter offsets from 5.15/6.1 references corrupts memory due to `wake_state` (+0x40) shifting downstream fields.
> - Borrowing `MCAST_WAITER_OFF` constants from 5.15 (`0x78`), Lenovo (`0x60`), or Huawei (`0x60`) causes the write to land outside `waiter->lock` on SM-X216B (which requires `0xc8`).
> - Every target kernel requires independent disassembly verification of both the futex allocation frame and the setsockopt copy frame.
