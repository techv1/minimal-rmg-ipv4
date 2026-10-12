# Reference Repositories Inventory and Evidence Index

**Document Version:** 1.0  
**Context:** CVE-2026-43499 / GhostLock MCAST Reference Study  
**Target Comparison Subject:** Samsung Galaxy Tab A9+ 5G (`SM-X216B`, `5.4.249-qgki-31053672-abX216BXXS9DYJ7`, Qualcomm SM6375)  

---

## 1. Upstream Repository Inventory Matrix

| # | Repository | Checked Commit / Tag | Target Device & SoC | Kernel Version & ABI | MCAST Option & Route | Claim Level | Key Files & Evidence Locations |
|---|------------|----------------------|---------------------|----------------------|----------------------|-------------|--------------------------------|
| **1** | [BuSung-dev/Root-My-Galaxy-Payloads](https://github.com/BuSung-dev/Root-My-Galaxy-Payloads) | `6e3223e` (`main`) | Samsung Galaxy Tab S9 (`SM-X710`), Tab S9 Ultra (`SM-X916B`), S23 Ultra (`SM-S918B`), S23+ (`SM-S916B`); Qualcomm Snapdragon 8 Gen 2 (kalama) | `5.15.189-android13-8` (GKI), AArch64; Compact waiter (88B, `0x58`) | `MCAST_JOIN_SOURCE_GROUP` (IPv6, optname 46); route: `mcast` | **Hardware-verified end-to-end** | `docs/SM-X916B-X916BXXS6EZG3.md` (§ "MCAST_WAITER_OFF derivation"); `docs/SM-X710-X710XXS6EZF1.md`; `src/slide_app.c` (`slide_mcast_stack_copy`) |
| **2** | [newazbenalam/Root-My-Galaxy-Payloads](https://github.com/newazbenalam/Root-My-Galaxy-Payloads) | `16b6008` (`main`) | Samsung Galaxy S21 FE 5G (`SM-G990B2` / `r9q`), Qualcomm Snapdragon 888 (SM8350) | `5.4` (`G990B2XXSJIZF1`), AArch64; Legacy waiter (80B, `0x50`, `LEGACY_RT_MUTEX_WAITER 1`) | No MCAST implemented (uses `pselect6` / `fd_set`); route: `pselect` | **Target-profile / Header derivation** | `src/targets/r9q-G990B2XXSJIZF1/target.h` (Lines 24-25, 131-137); `src/fops.c` (`do_pselect_fake_lock_route`); `src/slide_app.c` |
| **3** | [soumarcelino/Root-My-Galaxy-SM-S918B](https://github.com/soumarcelino/Root-My-Galaxy-SM-S918B) | `d152f19` (tag `v0.6.0-beta-2`) | Samsung Galaxy S23 Ultra (`SM-S918B` / `dm3q`), Qualcomm Snapdragon 8 Gen 2 (kalama) | `5.15.189-android13-8-33413713-abS918BXXSAFZF5`, AArch64; Compact waiter (88B, `0x58`) | Alternate routes: `pselect`, `sigreturn`, initial MCAST research; route: `slide_app` | **Hardware-verified end-to-end** | `targets/afzf5/payload/src/targets/dm3q-S918BXXSAFZF5/target.h` (Lines 33, 158-166); `targets/afzf5/payload/src/slide_app.c`; `targets/afzf5/payload/src/sigreturn.c` |
| **4** | [wonjj6768/Lenovo-TB365FC-GhostLock-CVE-2026-43499](https://github.com/wonjj6768/Lenovo-TB365FC-GhostLock-CVE-2026-43499) | `bc326e0` (`main`) | Lenovo Tab (`TB365FC` / `TB365FC_PRC`), MediaTek SoC | `5.15.170-android13-8-00003-g9a0b9d56c900-ab13225392`, AArch64; Compact waiter (88B, `0x58`) | `MCAST_BLOCK_SOURCE` (43, IPv4 UDP, `IPPROTO_IP`); `STAMP_OFF=0x60` | **Hardware-verified writer / PoC-to-cred** | `src/poc_mcast_root.c` (`mcast_stamp_stack` lines 457-487, `STAMP_OFF` lines 75-76); `src/target.h` (Compact waiter annotations) |
| **5** | [AnonymousUser369/GhostLockAdapt](https://github.com/AnonymousUser369/GhostLockAdapt) | `26ca85d` (`main`) | POCO air (`air-AP3A.240905.015.A2`), MediaTek / Qualcomm | `5.15.180`, AArch64; Compact waiter (88B, `0x58`, `WAKE_STATE@0x40`) | `MCAST_BLOCK_SOURCE` (43, IPv4 UDP, `IPPROTO_IP`); `WAITER_OFF=0x60`; also `sendmmsg` iovec spray | **Hardware-verified writer & probe calibration** | `exploit-finale/docs/WRITE-PRIMITIVES.md` (§ 1 & 2); `Target/waiter_field_offsets_5-15-180.md`; `poc-mcast-root/src/poc_mcast_root.c` |
| **6** | [zzzxxxxxxxxxx/GhostLock-GOT-W29](https://github.com/zzzxxxxxxxxxx/GhostLock-GOT-W29) | `cf3ca1e` (`main`) | Huawei MatePad Pro 11 (`GOT-W29`), Qualcomm Snapdragon 865 (SM8250 / kona) | `4.19.157-perf+`, AArch64; Vendor-extended waiter (88B, `0x58` via `CONFIG_HW_FUTEX_PI`) | `MCAST_BLOCK_SOURCE` (43, IPv4 UDP, `SOL_IP`); `MCAST_WAITER_OFF=0x60`; parked with `pselect6` | **PoC / Hardware-verified writer & QEMU verified** | `exploit/ghostlock-source/src/slide.c` (Lines 14-20, 48-50, 249-261); `exploit/ghostlock-source/src/target.h` (Lines 39-46); `firmware/` |
| **7** | [zhubaohe123/ghostlock-kit](https://github.com/zhubaohe123/ghostlock-kit) | `bc67322` (tag `v1.0.0`) | vivo iQOO Neo9S Pro (`V2339FA` / `PD2339`), MediaTek Dimensity 9300+ (`MT6989`) | `6.1.145-android14-11`, AArch64; Compact waiter (88B, `0x58`) | `MCAST_JOIN_SOURCE_GROUP` (46, IPv4 TCP/UDP, `IPPROTO_IP`); `MCAST_WAITER_OFF=0x60` | **Hardware-verified end-to-end** | `docs/2026-09-13_技术分析-mcast-transport.md` (§ 2-5); `docs/2026-09-13_技术分析-rtmutex-waiter与PI-walk.md`; `src/ghostlock-mcast.patch` |
| **8** | [NebuSec/CyberMeowfia](https://github.com/NebuSec/CyberMeowfia) | `94ae8ab` (`main`) | Google Pixel 6 / 8 / 9 family (tokay, comet, caiman, komodo, blazer, frankel, mustang, oriole), Google Tensor G1-G4 | `5.15` / `6.1` GKI, AArch64; Compact waiter (88B, `0x58`) | Upstream baseline: `pselect` route (`pselect6` with `fd_set`); no MCAST | **Architecture reference / Hardware-verified baseline** | `IonStack/CVE-2026-43499/exploit/src/slide.c` (`slide_pselect_stack_copy`); `IonStack/CVE-2026-43499/exploit/src/targets/` |

---

## 2. Detailed Reference Analysis and Evidence Tagging

### Reference 1: Samsung Root-My-Galaxy-Payloads (`BuSung-dev`)
- **Status / Claim:** Hardware-verified end-to-end on `SM-X710`, `SM-X916B`, `SM-S918B`, and `SM-S916B`.
- **Evidence Tag:** `VERIFIED FROM SOURCE` and `DOCUMENTED BY PROJECT AUTHOR`.
- **MCAST Option:** `setsockopt(AF_INET6, IPPROTO_IPV6, MCAST_JOIN_SOURCE_GROUP, ...)`. Note that this is **IPv6** multicast, calling `do_ipv6_setsockopt()`, allocating a 0x2c0 frame with local buffer `greqs` at `sp+0x40`.
- **Stack Geometry:** Calculated from entry `E`: Futex waiter at `E - 0x338`, MCAST buffer at `E - 0x3b0`, resulting in `MCAST_WAITER_OFF = 0x78` (120 bytes).
- **Target Kernel Comparison:** The Samsung 5.15 GKI tree introduced the 88-byte compact waiter (`wake_state` at `+0x40`, `ww_ctx` at `+0x50`), completely distinct from Linux 5.4 on `SM-X216B`.

### Reference 2: Samsung S21 FE Payload Feed (`newazbenalam`)
- **Status / Claim:** Target profile / Header derivation for `SM-G990B2` (`r9q-G990B2XXSJIZF1`).
- **Evidence Tag:** `VERIFIED FROM SOURCE`.
- **MCAST Status:** Does **not** contain an MCAST implementation; it uses `pselect6` (`SLIDE_PSELECT_WORD_SHIFT 0`).
- **Relevance:** Highly significant for **waiter layout**. It explicitly defines `LEGACY_RT_MUTEX_WAITER 1` and `FAKE_WAITER_LAYOUT_SIZE 0x50` (80 bytes), with `task` at `0x30`, `lock` at `0x38`, `prio` at `0x40`, and `deadline` at `0x48`, confirming the Samsung Linux 5.4 layout matches `SM-X216B`.

### Reference 3: Original Samsung AFZF5 Source (`soumarcelino`)
- **Status / Claim:** Hardware-verified end-to-end on `SM-S918B`.
- **Evidence Tag:** `VERIFIED FROM SOURCE` and `DOCUMENTED BY PROJECT AUTHOR`.
- **Mechanisms:** Implements the core RMG exploitation engine: KASLR discovery via tracefs, memory preparation via `mm_struct` slab grooming and SKB reclaim, fake ashmem `file_operations` staging, arbitrary physical read/write via pipe buffers, and escalation via `call_usermodehelper_exec_work`.

### Reference 4: Lenovo TB365FC MCAST PoC (`wonjj6768`)
- **Status / Claim:** Hardware-verified writer / PoC-to-cred on Lenovo TB365FC.
- **Evidence Tag:** `VERIFIED FROM SOURCE`.
- **MCAST Mechanism:** Uses IPv4 UDP `setsockopt(fd, IPPROTO_IP, MCAST_BLOCK_SOURCE, gsr, 264)` with `STAMP_OFF = 0x60`.
- **Payload Structure:** Statically prepares a 264-byte `group_source_req` buffer, placing fake waiter fields at `b + STAMP_OFF + field_offset`. Escalates via DirtyPipe cred patch rather than UMH/workqueue.

### Reference 5: GhostLockAdapt (`AnonymousUser369`)
- **Status / Claim:** Hardware-verified writer & probe calibration on POCO air (Linux 5.15.180).
- **Evidence Tag:** `VERIFIED FROM SOURCE` and `VERIFIED FROM DISASSEMBLY`.
- **MCAST Mechanism:** Uses IPv4 UDP `setsockopt(fd, IPPROTO_IP, MCAST_BLOCK_SOURCE, buf, 0x108)`. Disassembly proves `ip_setsockopt` executes `memset(sp+0x18, 0, 0x108)` followed by `copy_from_sockptr(sp+0x18, optval, 0x108)`.
- **Diagnostic Method:** Documents the dynamic probe method (`MCAST_PROBE_INDEX=1`) where filling slots with index identifiers allows reading the exact clobbered register from panic dumps (`x27`), resolving `WAITER_OFF = 0x60`.

### Reference 6: Huawei MatePad Pro 11 GOT-W29 (`zzzxxxxxxxxxx`)
- **Status / Claim:** PoC and hardware-verified writer on Huawei GOT-W29 (Linux 4.19.157).
- **Evidence Tag:** `VERIFIED FROM SOURCE` and `VERIFIED FROM DISASSEMBLY`.
- **MCAST Mechanism:** Uses IPv4 `setsockopt(fd, SOL_IP, MCAST_BLOCK_SOURCE, buf, 0x108)`. Copies `group_source_req` to `sp+0x20` inside `do_ip_setsockopt(0x190)`.
- **Stack Anchors:** Futex waiter placed at `E - 0x1b0`; copy destination at `E - 0x210`; `MCAST_WAITER_OFF = 0x60`. Waiter contains vendor fields `major_prio` (+0x40) and `major_only` (+0x44) due to Huawei `CONFIG_HW_FUTEX_PI`. Waiter thread then executes a blocking `pselect6` to park in-kernel without clobbering stack.

### Reference 7: vivo / iQOO Neo9S Pro MCAST Port (`zhubaohe123`)
- **Status / Claim:** Hardware-verified end-to-end on iQOO Neo9S Pro (Linux 6.1.145).
- **Evidence Tag:** `VERIFIED FROM SOURCE` and `VERIFIED FROM DISASSEMBLY`.
- **MCAST Mechanism:** Uses IPv4 `setsockopt(fd, IPPROTO_IP, MCAST_JOIN_SOURCE_GROUP, buf, 0x108)`. Demonstrates setting `buf[8] = 0` (`gsr_group.ss_family = 0`) to trigger `EADDRNOTAVAIL` (errno 99) early exit immediately following `copy_from_sockptr`, preventing actual multicast state modifications.
- **Stack Geometry:** System call entry `S'`: MCAST buffer at `S' - 0x338`, waiter at `S' - 0x2D8`, yielding `MCAST_WAITER_OFF = 0x60`.

### Reference 8: Upstream IonStack Reference (`CyberMeowfia`)
- **Status / Claim:** Upstream architectural reference (Linux 5.15 / 6.1 on Google Tensor).
- **Evidence Tag:** `VERIFIED FROM SOURCE`.
- **Carrier:** Native `pselect6` with `fd_set` manipulation.
- **Architecture Value:** Defined the multi-stage post-corruption pipeline: (1) KASLR slide leak, (2) Waiter UAF triggering and fake waiter placement, (3) `rb_erase` left-only write primitive, (4) `mm_struct` slab reclaim and SKB grooming, (5) fake `file_operations` and pipe buffer arbitrary read/write, and (6) `call_usermodehelper_exec_work` escalation.

---

## 3. Discrepancy & Ambiguity Register

1. **IPv4 vs IPv6 MCAST Discrepancy:**
   - Samsung 5.15 references (`BuSung-dev`: X710, X916B, S918B) use **IPv6** multicast (`AF_INET6`, `IPPROTO_IPV6`, `MCAST_JOIN_SOURCE_GROUP`), resulting in `MCAST_WAITER_OFF = 0x78`.
   - GhostLockAdapt explicitly proved that on their 5.15 kernel, IPv6 call chains are ~0xE0 deeper, causing the waiter to land outside the 264-byte copy buffer (`WAITER_OFF ≈ 0xF8`), forcing them to use **IPv4** (`MCAST_BLOCK_SOURCE`).
   - `SM-X216B` (Linux 5.4) uses **IPv4** native 64-bit MCAST, which lands the waiter base at offset `0xc8` (200 bytes) inside the 264-byte buffer.

2. **Wait Buffer Size & Reachability:**
   - Several references note that `MCAST_JOIN_GROUP` (optname 42) uses `struct group_req` (136 bytes), which falls short of the dangling waiter by 64 bytes on Linux 5.4.
   - Only `MCAST_BLOCK_SOURCE` (43) or `MCAST_JOIN_SOURCE_GROUP` (46), which take `struct group_source_req` (264 bytes / `0x108`), provide sufficient depth to cover `waiter->task` (+0x30) and `waiter->lock` (+0x38).
