# GhostLock / MCAST Reference Agent Prompt + Safe Port Guide

## Purpose

Use this document as an agent prompt for **source-code study and authorized kernel-security research** on CVE-2026-43499 / GhostLock, with particular focus on the Linux IPv4 MCAST stack-writer concept and comparison against the Samsung Galaxy Tab A9+ 5G `SM-X216B`.

### Target under study

- Device: Samsung Galaxy Tab A9+ 5G
- Model: `SM-X216B`
- Codename: `gta9p`
- Firmware: `X216BXXS9DYJ7`
- Kernel: `5.4.249-qgki-31053672-abX216BXXS9DYJ7`
- SoC: Qualcomm Snapdragon 695 / SM6375
- Architecture: AArch64
- Local evidence files:
  - `smx216b_mcast_geometry_report.md`
  - `kernel_profile_verified.json`
  - `target.h`

## Agent prompt

You are a kernel-security research/code-analysis agent. Build a **reference-only comparative study** of GhostLock/CVE-2026-43499 MCAST implementations and produce a non-operational porting analysis for `SM-X216B`.

### Source repositories and exact reference folders

1. **Samsung Root-My-Galaxy-Payloads**
   - `docs/SM-X710-X710XXS6EZF1.md`
   - `docs/SM-X916B-X916BXXS6EZG3.md`
   - `docs/SM-S918B-S918BXXSAFZF5.md`
   - `docs/SM-S916B-S916BXXSAFZG1.md`
   - `docs/PORTING.md`
   - `src/targets/`
   - `https://github.com/BuSung-dev/Root-My-Galaxy-Payloads`

2. **Samsung S21 FE payload feed**
   - `src/targets/r9q-G990B2XXSJIZF1/`
   - `docs/`
   - `support/`
   - `https://github.com/newazbenalam/Root-My-Galaxy-Payloads`

3. **Original Samsung AFZF5 source**
   - `targets/afzf5/payload/src/`
   - `https://github.com/soumarcelino/Root-My-Galaxy-SM-S918B`

4. **Lenovo TB365FC MCAST PoC**
   - `src/poc_mcast_root.c`
   - `https://github.com/wonjj6768/Lenovo-TB365FC-GhostLock-CVE-2026-43499`

5. **GhostLockAdapt**
   - `poc-mcast-root/src/`
   - `poc-mcast/`
   - `exploit-finale/docs/`
   - `Target/`
   - `https://github.com/AnonymousUser369/GhostLockAdapt`

6. **Huawei MatePad Pro 11 GOT-W29**
   - `exploit/ghostlock-source/`
   - `firmware/`
   - `tools/`
   - `https://github.com/zzzxxxxxxxxxx/GhostLock-GOT-W29`

7. **vivo / iQOO Neo9S Pro MCAST port**
   - `src/ghostlock-mcast.patch`
   - `README.md`
   - `https://github.com/zhubaohe123/ghostlock-kit`

8. **Upstream IonStack reference**
   - `IonStack/CVE-2026-43499/exploit/`
   - `https://github.com/NebuSec/CyberMeowfia`

### Task 1 — Clone and inventory

Clone each repository read-only. Do not modify upstream repositories.

Create `references/INDEX.md` containing for each reference:

- commit/tag checked
- device/model
- kernel version
- architecture
- SoC
- MCAST option used
- route name
- relevant files
- whether the source claims geometry-only, PoC-only, hardware-verified writer, or end-to-end validation
- exact evidence location (file + function/section)

### Task 2 — Isolate the MCAST mechanism

For each MCAST reference, trace only the conceptual path:

`userspace setsockopt` → IPv4 socket option handler → `group_source_req`-related copy → kernel-stack placement → relationship to `rt_mutex_waiter`

Explain:

- why the multicast API is useful as a stack-data carrier;
- which MCAST option enters the relevant code path;
- what kernel-side local structure receives the userspace data;
- which stack frame owns that local structure;
- how each project measures/represents stack geometry;
- which pieces are generic vs kernel-build-specific.

Do not generate exploit payload bytes or a weaponized stack-stamping routine.

### Task 3 — Compare waiter layout

Build a source-backed matrix for:

- Huawei 4.19
- Samsung G990B2 5.4
- Samsung SM-X216B 5.4
- Samsung 5.10 references
- Samsung X710/X916B/S918B/S916B 5.15
- Lenovo/GhostLockAdapt 5.15
- iQOO/Vivo 6.1

Record:

- `struct rt_mutex_waiter` size
- `tree_entry`
- `pi_tree_entry`
- `task`
- `lock`
- other fields that materially affect interpretation

Distinguish verified disassembly facts from inference.

### Task 4 — Compare call chains

For each reference, document the MCAST and futex call-chain shape at a conceptual level, e.g.:

`syscall entry → setsockopt → sock_common_setsockopt → ip_setsockopt → do_ip_setsockopt`

and compare with:

`syscall entry → __arm64_sys_futex → do_futex → PI waiter creation/cleanup`

Do not turn this into a timing recipe or exploitation procedure.

### Task 5 — Use the supplied SM-X216B evidence as the target-side source of truth

Read:

- `smx216b_mcast_geometry_report.md`
- `kernel_profile_verified.json`
- `target.h`

Create `references/SM-X216B_REFERENCE_BASELINE.md` containing:

- kernel identity
- verified relevant symbols
- verified `rt_mutex_waiter` layout
- verified MCAST call-chain geometry
- differences from 5.15 Samsung profiles
- unresolved questions

Do not silently alter or replace supplied measurements. Label any new inference explicitly.

### Task 6 — Determine the closest references

Create a comparison table with separate fields for:

1. kernel-generation similarity
2. Samsung/Qualcomm similarity
3. MCAST implementation similarity
4. stack-frame similarity
5. usefulness as a study reference

Do not call a reference “portable” merely because it uses MCAST.

### Task 7 — Produce a safe porting guide

Create `docs/PORTING_GUIDE.md`.

It must be a **research workflow**, not a ready-to-run exploit.

Required sections:

1. Scope and authorization
2. Target identity and evidence inventory
3. Reference-repository map
4. How to compare MCAST stack geometry
5. How to compare `rt_mutex_waiter` layouts
6. How to compare `do_futex()` and `do_ip_setsockopt()` frames
7. How to distinguish writer/carrier, kernel primitive, memory preparation, post-corruption stage, and root-management handoff
8. Kernel/compiler/config compatibility checklist
9. Evidence requirements for calling a route “verified”
10. Failure-analysis taxonomy
11. Emulator/QEMU or offline-vmlinux validation methodology
12. Regression checklist after kernel/vendor changes
13. What must remain target-specific

### Task 8 — Code map

Create `references/MCAST_CODE_MAP.md`.

For each project include:

- exact source file
- relevant function names
- role of each function
- call relationship
- MCAST option
- data structure involved
- stack-frame relevance
- confidence level

Use tables and small ASCII diagrams.

### Task 9 — Do not overclaim

Treat every statement as one of:

- VERIFIED FROM SOURCE
- VERIFIED FROM DISASSEMBLY
- DOCUMENTED BY PROJECT AUTHOR
- INFERENCE
- UNKNOWN

When repositories disagree, preserve both facts and explain the discrepancy.

### Safety boundary

This is a **code-analysis and defensive/authorized research task**.

Do NOT produce:

- a working kernel exploit
- a device-specific privilege-escalation payload
- exact fake waiter contents intended to obtain a kernel write
- heap-grooming/spray instructions
- race timing values or CPU pinning recipes intended to reproduce exploitation
- kernel-address substitution intended to bypass protections
- credential/SELinux/seccomp patch instructions
- a one-click rooting script
- instructions for bypassing Android security controls

It is fine to explain the vulnerability architecture, compare source code, inspect disassembly, document structure layouts already supplied, and identify how references differ from the target.

### Required deliverables

- `references/INDEX.md`
- `references/SM-X216B_REFERENCE_BASELINE.md`
- `references/MCAST_CODE_MAP.md`
- `docs/PORTING_GUIDE.md`
- `docs/MCAST_COMPARISON.md`

`docs/MCAST_COMPARISON.md` must answer, with evidence:

> Which of Huawei, G990B2, X710, X916B, S918B, S916B, Lenovo, GhostLockAdapt, and iQOO/Vivo is closest to SM-X216B for (a) kernel generation, (b) MCAST implementation, and (c) Samsung/Qualcomm stack geometry?

Do not infer portability from those comparisons.

---

# PORTING_GUIDE.md — recommended contents

## Scope

Authorized source/disassembly study of CVE-2026-43499 and the MCAST stack-writer concept. Not a recipe for producing a privilege-escalation exploit.

## Target baseline

`SM-X216B / gta9p / X216BXXS9DYJ7`

`5.4.249-qgki-31053672-abX216BXXS9DYJ7`

AArch64 / Qualcomm SM6375.

Treat the supplied SM-X216B report/profile/header as authoritative target-side evidence.

## Reference strategy

| Reference | Primary purpose |
|---|---|
| Huawei GOT-W29 | older-kernel MCAST/GhostLock research |
| G990B2 | Samsung Qualcomm Linux 5.4 comparison |
| X710 | modern Samsung MCAST writer reference |
| X916B | detailed Samsung MCAST geometry derivation |
| S918B | Samsung MCAST + alternate writer architecture |
| S916B | related Samsung 5.15 profile |
| Lenovo | compact standalone MCAST PoC |
| GhostLockAdapt | MCAST vs alternate carrier research |
| iQOO/Vivo | MCAST transport substitution on 6.1 |
| IonStack | upstream/common architecture |

## MCAST research workflow

1. Identify the exact kernel build.
2. Obtain matching `vmlinux` and config.
3. Confirm the IPv4 multicast setsockopt handler.
4. Identify the local structure receiving the userspace multicast request.
5. Record the stack-allocation sequence.
6. Independently map the futex PI waiter location.
7. Compare the two stack frames as geometry rather than portable constants.
8. Document the conceptual overlap.
9. Validate assumptions against disassembly and symbols.
10. Keep writer, kernel primitive, memory preparation, post-corruption stage, and root-management handoff separate.

## Compatibility checklist

Compare at minimum:

- Linux version
- Android GKI/QGKI status
- architecture
- compiler family/version
- LTO configuration
- CFI/Shadow Call Stack
- kernel config
- vendor backports
- `rt_mutex_waiter` layout
- futex call-chain shape
- IPv4 multicast implementation
- stack frame size/alignment
- mitigation configuration

## Evidence hierarchy

Strongest to weakest:

1. matching `vmlinux` disassembly
2. matching symbol table
3. matching config
4. matching vendor kernel source
5. project documentation
6. code comments
7. inference

## Validation language

Always distinguish:

- MCAST path exists
- MCAST geometry appears compatible
- writer behavior was experimentally observed
- full privilege escalation was hardware-verified

These are different claims.

## Failure taxonomy

- wrong kernel build
- different waiter layout
- different stack frame
- compiler-generated frame difference
- different multicast handler
- incompatible kernel config
- later-stage incompatibility
- insufficient evidence

## Final rule

Do not port numerical values, offsets, timing constants, or post-corruption primitives from another device solely because the other project uses the same MCAST option. Derive target-specific conclusions independently from the matching kernel evidence.
