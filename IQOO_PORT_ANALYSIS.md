# iQOO Z9 5G Reference vs Current SM-X216B Harness

This note compares the current `minimal-rmg-ipv4` workspace with the reference tree at:

`/root/s/references/iQOO-Z9_5G-vivo- research`

No source changes are implied by this document. It is an analysis of the gap between the two codebases.

## Executive Summary

The current workspace is a small SM-X216B Android 5.4 test harness centered on an IPv4 MCAST stack stamp and a stubbed UMH/root stage. The iQOO reference is a complete Android 5.15 target-specific payload with a different target profile, different stack writer strategy, broader KASLR/physical oracle machinery, app/adb launch integration, and KernelSU late-load flow.

This is not a constants-only port. Some constants can be copied or compared, but the core control path differs enough that blindly replacing `target.h` would leave the local harness internally inconsistent.

## Repository Shape

| Area | Current `minimal-rmg-ipv4` | iQOO reference |
| --- | --- | --- |
| Project type | Minimal native C harness | Android app plus native payload |
| Primary output | Static `/init` and initramfs for lab/QEMU-style boot | Shared app payload, root helper, APK-facing artifacts |
| Build driver | `build.sh` using direct NDK clang invocation | `payload/Makefile` with separate preload, app payload, and helper targets |
| Main entry | `main.c` as PID 1 harness or direct worker | `payload/src/main.c` plus APK/Shizuku helper flow |
| Target profile | Flat `target.h` and `common.h` constants | `payload/src/targets/iqoo-z9-5g/target.h` selected through `TARGET_HEADER` |
| Root stage | `umh_root.c` logs intended UMH addresses, does not perform writes | Full payload contains extra root, fops, pipe, slide, and helper components |

## Target Identity

The current harness is labeled for:

- Device: Samsung Galaxy Tab A9+ 5G, `SM-X216B / gta9p`
- Kernel: `5.4.249-qgki-31053672-abX216BXXS9DYJ7`
- SoC family: Qualcomm SM6375
- Kernel image base: `0xffffffc010080000`

The iQOO reference is labeled for:

- Device: iQOO Z9 5G `I2302` and vivo T3 5G `V2334`
- Kernel: `5.15.178-android13-8-g0ebe6a5da65d`
- SoC family: MediaTek MT6886 / Dimensity 7200
- Kernel image base: `0xffffffc008000000`

The reference README treats iQOO Z9 5G and vivo T3 5G as the same payload target because their kernel release, ABI, offsets, and KernelSU pairing match. That equivalence does not extend to SM-X216B.

## Kernel Layout Differences

The local harness assumes an Android 5.4 layout:

- `rt_mutex_waiter` size: `0x50`
- waiter `task`: `0x30`
- waiter `lock`: `0x38`
- waiter `prio`: `0x40`
- waiter `deadline`: `0x48`
- no `wake_state` or `ww_ctx` field in the modeled waiter
- pipe buffer slots: `16`
- workqueue `WQ_DFL_PWQ_OFF`: `0xa0`
- task PI offsets around `0x8dc` through `0x900`

The iQOO profile uses Android 5.15-style assumptions:

- compact waiter layout enabled by `COMPACT_RT_MUTEX_WAITER`
- waiter `task`: `0x30`
- waiter `lock`: `0x38`
- waiter `wake_state`: `0x40`
- waiter `prio`: `0x44`
- waiter `deadline`: `0x48`
- waiter `ww_ctx`: `0x50`
- waiter layout size: `0x58`
- pipe buffer slots: `32`
- workqueue `WQ_DFL_PWQ_OFF`: `0xb0`
- task PI offsets around `0x884` through `0x8b0`

This difference matters because the local race code writes only the 5.4-shaped `task` and `lock` fields through an IPv4 stack buffer. The iQOO reference models more 5.15 fields and has different timing/route assumptions around the stale waiter.

## Stack Writer Difference

The current harness is built with:

```sh
-DUSE_MCAST_STAMP
```

Its `futex_race.c` uses native IPv4 `MCAST_BLOCK_SOURCE` with a 264-byte buffer. The comments in `target.h` describe a 5.4-specific overlap:

- waiter base: `SP_sys - 0x1e8`
- MCAST buffer: `SP_sys - 0x2b0 ... SP_sys - 0x1a8`
- waiter base inside buffer: `0xc8`

The iQOO reference uses a target-selected writer:

```make
STACK_WRITER ?= 2
```

and defines:

- `SLIDE_STACK_WRITER_MCAST 1`
- `SLIDE_STACK_WRITER_SIGRETURN 2`
- `MCAST_WAITER_OFF 0x98`
- `SIGRETURN_FPSIMD_WAITER_OFF 0x08`
- `SIGRETURN_GROW_SVE 1`

The default iQOO path is SIGRETURN-shaped, not the local IPv4 MCAST path. That is the largest architectural mismatch.

## KASLR And Address Resolution

The current workspace has `kaslr.c` stubbed:

```c
uint64_t resolve_slide(uint64_t physical_base) {
    (void)physical_base;
    return 0;
}
```

It mostly uses static target addresses from `target.h`.

The iQOO reference has substantial slide-resolution machinery:

- tracefs based slide path
- physical/P0 oracle path
- P0 fingerprint table
- MediaTek physical placement assumptions
- PAC-aware trace parsing constants
- large slide candidate range for RELR-relocated kernels

The local `p0_fingerprint.h` appears to be related to an iQOO-style P0 fingerprint concept, but the rest of this repository does not contain the iQOO reference's complete slide/oracle implementation.

## Symbol Offset Differences

Representative offsets differ substantially:

| Symbol | Current SM-X216B | iQOO reference |
| --- | ---: | ---: |
| `KIMAGE_TEXT_BASE` | `0xffffffc010080000` | `0xffffffc008000000` |
| `INIT_TASK_OFF` | `0x02960280` | `0x02d23580` |
| `SYSTEM_UNBOUND_WQ_OFF` | `0x028ef9c8` | `0x02be07d8` |
| `CALL_USERMODEHELPER_EXEC_WORK_OFF` | `0x00297274` | `0x0018a30c` |
| `CALL_USERMODEHELPER_EXEC_WORK_CFI_JT_OFF` | `0x01601870` | `0x0178ade0` |
| `ANON_PIPE_BUF_OPS_OFF` | `0x021755b8` | `0x0201edf0` |
| `ASHMEM_FOPS_OFF` | `0x0214ee28` | `0x0219c2d8` |
| configfs write path | `CONFIGFS_WRITE_BIN_FILE_*` | `CONFIGFS_BIN_WRITE_ITER_*` |

The names also differ, not just the numeric values. The current target profile names Linux 5.4 configfs file helpers, while the iQOO reference uses Linux 5.15 iterator-style configfs helpers.

## Runtime Flow Difference

Current local flow:

1. `main.c` optionally acts as PID 1.
2. Worker drops to UID/GID 2000.
3. `trigger_futex_race()` runs the race harness.
4. `phase2_bridge_diag()` checks the ashmem/configfs bridge prerequisites without attempting credential or workqueue writes.
5. No credential change is performed by the current root stage.

iQOO reference flow:

1. Android app or adb shell launches the helper.
2. Helper validates target model/kernel/ABI in the app path.
3. Payload resolves slide and prepares memory primitives through its own route.
4. Payload coordinates fops/pipe/configfs components.
5. Helper late-loads a matching KernelSU daemon artifact.

The local Phase 2 path is a diagnostic gate relative to the reference. It checks ashmem availability, benign ashmem naming behavior, and the resolved bridge target constants. It does not contain the reference's fops handoff, pipe bridge, credential patch, workqueue queueing, or KernelSU helper integration.

## Build And Artifact Differences

Current `build.sh`:

- hard-codes `WORKDIR=/root/s/minimal-rmg-ipv4`
- writes to `/root/s/ipv4-test-artifacts`
- builds one static binary named `init`
- packages an initramfs
- uses `-Os`, static linking, and `-DUSE_MCAST_STAMP`

iQOO `payload/Makefile`:

- expects `ANDROID_NDK_HOME`
- uses API 35
- builds a preload `.so`
- builds an app payload `.so`
- builds a PIE root helper
- passes `TARGET_HEADER="targets/iqoo-z9-5g/target.h"`
- passes `APP_PAYLOAD=1` and `SLIDE_STACK_WRITER=$(STACK_WRITER)` for the app payload
- verifies a prebuilt `ksud-iqoo-z9-5g` artifact

The build model is therefore also not directly compatible.

## Current Workspace Items That Already Resemble iQOO

Some hints in the local workspace already point toward earlier iQOO/RMG adaptation:

- `main.c` mentions the iQOO/A17 pattern of keeping PID 1 alive.
- `futex_race.c` mentions adopting the iQOO/RMG pattern of not joining background threads.
- `p0_fingerprint.h` uses a P0 fingerprint table concept similar to the reference.

Those are isolated pieces. They do not make the whole workspace an iQOO port.

## Main Porting Gap

A direct port would need an explicit design decision first:

1. Keep this repository as an SM-X216B 5.4 IPv4 MCAST harness and only document iQOO differences.
2. Convert this repository into an iQOO-like 5.15 payload, which means replacing more than constants.
3. Keep both targets side by side, with target-specific headers and build flags selected at build time.

The third option is the cleanest engineering direction if this workspace needs to preserve SM-X216B while studying the iQOO reference. It avoids mixing 5.4 and 5.15 assumptions in the same `common.h` and `target.h`.

## Files Most Affected By A Real Port

If code changes are later approved, these files would need review before editing:

- `target.h`: target identity, symbol offsets, fops/configfs names, waiter layout.
- `common.h`: direct map, physical layout, waiter/task/workqueue offsets, pipe slots.
- `futex_race.c`: stack writer path and waiter field population.
- `kaslr.c`: currently stubbed; incompatible with the iQOO reference's slide logic.
- `pipe_rw.c`: modeled as Linux 5.4 configfs bridge; iQOO uses 5.15 iterator naming and offsets.
- `umh_root.c`: currently a placeholder that logs addresses.
- `build.sh`: currently builds the 5.4 IPv4 harness, not the iQOO payload shape.

## Suggested Next Step

Before editing code, decide whether the goal is:

- a clean comparative research note,
- a dual-target build structure,
- or an iQOO-only replacement of the current harness.

The safest immediate engineering step is to split target constants into separate target headers and make the build select one target explicitly. That would let the project represent the SM-X216B and iQOO profiles without cross-contaminating 5.4 and 5.15 assumptions.

## GhostLockAdapt `poc-mcast-root` Comparison

The most relevant GhostLockAdapt reference for this workspace is:

`/root/s/references/GhostLockAdapt/poc-mcast-root`

This is closer to the local repo than the iQOO Android app because it is also a standalone native C payload built around an MCAST stack stamp. It is still not a direct match: `poc-mcast-root` targets POCO/Redmi `air` on Android 15 with a 5.15.180 GKI kernel, while this workspace targets SM-X216B on Android 5.4.

### Project Shape

| Area | Current `minimal-rmg-ipv4` | GhostLockAdapt `poc-mcast-root` |
| --- | --- | --- |
| Target | Samsung SM-X216B | POCO/Redmi `air`, build `AP3A.240905.015.A2` |
| Kernel | `5.4.249-qgki-31053672` | `5.15.180-android13-8-00021-g46a5565a0982-ab13743836` |
| Build output | static `init` plus initramfs | static `poc_mcast_root` binary |
| Build script | `build.sh` with local NDK r28 path | `build.sh` with GhostLockAdapt NDK r29 path |
| Entry source | `main.c` plus `futex_race.c` | single primary `poc_mcast_root.c` plus helpers |
| Target profile | `target.h` and `common.h` are mixed into the root | `src/target.h`, `src/common.h`, and redirected `src/offset.h` |
| Root stage | `umh_root.c` is only a placeholder | contains a full escalation pursuit, plus investigation notes about instability |

### Why `poc-mcast-root` Is More Similar Than iQOO

Both this workspace and `poc-mcast-root` are native harnesses rather than Android app projects. Both use:

- futex PI topology as the entry condition,
- IPv4 MCAST stack stamping,
- direct NDK static builds,
- a small number of C files instead of an APK/runtime-download architecture,
- device-specific target constants in headers.

That makes `poc-mcast-root` useful as a structural reference for organizing a native proof harness. It is less useful as a source of drop-in offsets because its kernel family, target phone, and exploit stage assumptions differ.

### Major Technical Differences

The local SM-X216B code uses a Linux 5.4 waiter model:

- waiter size `0x50`
- `prio` at `0x40`
- no modeled `wake_state` or `ww_ctx`
- pipe buffer slots set to `16`
- workqueue offset `WQ_DFL_PWQ_OFF = 0xa0`

`poc-mcast-root` uses a Linux 5.15.180 compact waiter model:

- waiter layout size `0x58`
- `wake_state` at `0x40`
- `prio` at `0x44`
- `deadline` at `0x48`
- `ww_ctx` at `0x50`
- pipe buffer slots set to `32`
- 5.15-style task, cred, seccomp, and configfs offsets

The MCAST concept overlaps, but the field layout does not.

### MCAST Path Difference

The local harness hard-codes the SM-X216B 5.4 overlap model in `target.h`:

- MCAST option: IPv4 `MCAST_BLOCK_SOURCE`
- buffer size: `264`
- waiter base offset in buffer: `0xc8`
- task field offset in buffer: `0xf8`
- lock field offset in buffer: `0x100`

`poc-mcast-root` treats its MCAST offset as POCO-specific and references a proven 5.15.180 value in comments. It also carries probe/debug support around that offset. That probe-driven approach is the useful design lesson: stack overlap offsets should be treated as target-derived, not globally portable.

### KASLR And Symbol Resolution

The local repo has a stubbed KASLR resolver:

```c
uint64_t resolve_slide(uint64_t physical_base) {
    (void)physical_base;
    return 0;
}
```

`poc-mcast-root` has target-specific symbol resolution and multiple leak/support components:

- static symbol offset table for the POCO kernel,
- tracefs scheduling leak path,
- perf fallback helper,
- KernelSnitch-related support code,
- direct map and P0 constants for a 39-bit VA GKI layout.

That is a much fuller runtime environment than the local harness. The local code would need a real SM-X216B slide/address strategy before any later stage could be considered reliable.

### Configfs / Ashmem Difference

The local `pipe_rw.c` is written as a configfs/ashmem bridge stub for Linux 5.4-style offsets. It opens `/dev/ashmem` and exposes `kwrite64()` / `kread64()` wrappers, but it does not establish a verified working kernel read/write channel.

`poc-mcast-root` explicitly records that the configfs ashmem path is not viable on its POCO 5.15.180 target and stubs older configfs helpers for compatibility. Its active direction is therefore different from both:

- the local SM-X216B UMH placeholder, and
- the iQOO reference's more complete configfs/fops-oriented payload.

This is an important warning: even within the same GhostLock family, the working post-entry primitive is device/kernel-specific.

### Root Stage Difference

The local `umh_root.c` currently:

- writes a small shell script,
- prints the configured UMH function and workqueue addresses,
- returns without actually changing process credentials.

`poc-mcast-root` contains a real root pursuit, plus notes showing unresolved reliability problems. Its own `SELINUX_WRITE_INVESTIGATION.md` says a write can succeed but still leave the device unstable later because of side effects in the tree manipulation path. That makes it valuable as a cautionary reference: "the first visible write works" is not the same as "the target is stable."

### Offset Comparison

Representative constants:

| Constant | Current SM-X216B | `poc-mcast-root` POCO air |
| --- | ---: | ---: |
| `KIMAGE_TEXT_BASE` | `0xffffffc010080000` | `0xffffffc008000000` |
| `INIT_TASK_OFF` | `0x02960280` | `0x02c43640` |
| `ANON_PIPE_BUF_OPS_OFF` | `0x021755b8` | `0x01f85130` |
| `ASHMEM_FOPS_OFF` | `0x0214ee28` | `0x021027c8` |
| configfs write helper | `CONFIGFS_WRITE_BIN_FILE_*` | `CONFIGFS_BIN_WRITE_ITER_*` |
| direct map base | `0xffffffc000000000` | `0xffffff8000000000` |
| physical offset | `0x80000000` | `0x80000000` |
| pipe buffer slots | `16` | `32` |
| waiter size | `0x50` | `0x58` |

These values show that the target profiles are related only at the exploit-family level. They should remain separate headers.

### Useful Lessons From `poc-mcast-root`

The useful engineering ideas to carry over are structural:

- Keep target constants isolated in target files.
- Treat MCAST waiter overlap as target-derived and probe-verified.
- Keep KASLR/slide resolution separate from the race topology.
- Record kernel-source/BTF/disassembly contradictions in documentation.
- Track stability separately from first-stage success.
- Make build scripts clearly name their target and output.

The less portable parts are the actual offsets, field layouts, symbol table, and post-entry escalation path.

### What This Means For This Repo

Compared with both iQOO and `poc-mcast-root`, this workspace is best described as:

- a minimal SM-X216B 5.4 race/stamp harness,
- with some iQOO/RMG-inspired lifecycle comments,
- with no complete KASLR implementation,
- with no verified kernel read/write bridge,
- with a root stage that is explicitly incomplete.

The clean next step is still a target split:

```text
src/targets/sm-x216b/target.h
src/targets/iqoo-z9-5g/target.h
src/targets/poco-air-5.15.180/target.h
```

Then the build can select one target explicitly instead of mixing SM-X216B, iQOO, and POCO assumptions in the same global headers.

## Three-Stage Chain And Current Failure Point

The important common lesson from the iQOO reference and GhostLockAdapt references is that the futex race is not root execution by itself. It is the entry condition for a later kernel write primitive.

At a high level, the reference chain is:

```text
Stage 1: futex PI race + stack stamp
    -> fake waiter is consumed by the PI-chain walker
    -> rb-tree manipulation produces a single controlled qword write

Stage 2: use the write to establish a broader kernel R/W path
    -> target depends on kernel/device
    -> examples include fops/configfs/ashmem-style bridges or other page/pipe routes

Stage 3: use kernel R/W for root effect
    -> patch credentials, alter SELinux-related state, or trigger a helper path
```

The current SM-X216B logs only prove the beginning of Stage 1:

```text
CMP_REQUEUE_PI -> ret=-1 errno=35 (Resource deadlock would occur)
CMP_REQUEUE_PI: EDEADLK confirmed
```

That is useful because it shows the PI-cycle topology reaches the expected `EDEADLK` condition. It does not prove the fake waiter was consumed. The same logs show the waiter later times out and unlocks before the MCAST stamp:

```text
WAITER: FUTEX_WAIT_REQUEUE_PI -> ret=-1 errno=110 (Connection timed out)
WAITER: timeout detected, unlocking pi_chain
WAITER: UNLOCK_PI(pi_chain) -> ret=0
WAITER: entering IPv4 MCAST_BLOCK_SOURCE stamp
```

That ordering means the current run stamps after the futex state has already unwound. The later line:

```text
CONSUMER: sched_setattr -> ret=0
```

only proves the consumer syscall returned cleanly. It does not prove `rt_mutex_adjust_prio_chain` read the stamped waiter or performed a controlled qword write.

## Why The Previous UID 0 Run Was Not Proof

One run started as UID 0:

```text
[PRE-CHECK] Initial UID: 0, EUID: 0
[INFO] This initramfs starts the test as UID 0; UID 0 is not proof of escalation.
```

The final UID 0 result in that run therefore only means the process stayed root:

```text
[RESULT] Final UID: 0, GID: 0
```

It does not demonstrate escalation. A valid root test needs a non-root pre-check, such as UID/EUID 2000, followed by a verified transition after Stage 3. The earlier worker run did start as UID 2000 and ended as UID 2000, which confirms that no credential-changing stage was active.

## Stage Mapping: References vs Current Workspace

| Stage | iQOO reference | GhostLockAdapt `poc-mcast-root` | Current SM-X216B workspace |
| --- | --- | --- | --- |
| Stage 1: PI race | Full coordinated route around target profile | Standalone futex/MCAST route with probe/debug support | Reaches `EDEADLK`; diagnostic release now lets the waiter stamp before the consumer walk |
| Stack writer | target-selected, default SIGRETURN for iQOO app payload | IPv4 MCAST path, plus additional writer experiments in source | IPv4 MCAST only |
| Controlled write proof | integrated into later fops/pipe path | investigated through ghost-write and stability logs | no marker/probe proof yet |
| Stage 2: broader R/W | fops/configfs/pipe-oriented route | configfs path treated as unavailable on POCO; DirtyPipe-style route explored | `pipe_rw.c` exists but is not a verified bridge |
| Stage 3: root effect | helper/KernelSU-oriented flow | root pursuit with reliability caveats | no root-effect stage is active |

## Approach A vs B In This Context

Approach A, direct credential patching, depends on having a real kernel R/W primitive and exact SM-X216B `task_struct` / `cred` offsets. It is usually the simpler root-effect design once Stage 2 is real because it avoids workqueue object choreography and usermode-helper constraints.

Approach B, workqueue UMH, depends on a real kernel R/W primitive plus exact workqueue and `subprocess_info` layout, valid object staging, and CFI-compatible function pointers. On this kernel family, `CONFIG_CFI_CLANG=y`, `CONFIG_CFI_CLANG_SHADOW=y`, and `CONFIG_STATIC_USERMODEHELPER=y` make that path more sensitive. The configured `CONFIG_STATIC_USERMODEHELPER_PATH=""` means UMH behavior should be verified from the kernel code before relying on a dynamic `/data/local/tmp/root.sh` execution model.

For this workspace, neither Approach A nor B is the immediate missing piece. The immediate missing piece is a demonstrated Stage 1 to Stage 2 handoff:

```text
EDEADLK -> waiter released without timeout cleanup -> stack stamp -> consumer PI walk -> marker/write proof
```

Only after that evidence exists does it make sense to choose between credential patching and UMH as the final root effect.

## Current Diagnostic Ordering

The current diagnostic harness has been adjusted to match the reference ordering at the scheduling level without enabling a root-effect stage:

```text
CMP_REQUEUE_PI -> EDEADLK
DIAG_RELEASE wakes the waiter
waiter returns without timeout cleanup
IPv4 MCAST stamp completes
consumer sched_setattr walk runs
Stage 2 is skipped because marker/write proof is still missing
```

The important change from the older run is that the waiter no longer times out and unlocks `pi_chain` before the stamp. The expected successful diagnostic classification is now:

```text
DIAG: classification=stamp-before-walk-unverified
DIAG: stage1_result=INCONCLUSIVE marker/write proof still required
```

That classification means the timing/order now resembles the references more closely, but the port is still not proven. The missing proof is a harmless marker or equivalent evidence showing that the stamped data was consumed as the intended waiter state and produced the expected controlled effect. Until that proof exists, `main.c` intentionally skips Stage 2 and reports the final non-root UID/GID.

## Required Evidence Before Calling The Port Working

A meaningful SM-X216B validation log should show:

- a non-root starting UID/EUID,
- the waiter avoiding timeout cleanup before the stamp,
- a probe or marker proving the MCAST buffer overlaps the waiter fields,
- the consumer walk acting on the stamped waiter,
- one controlled qword write to a harmless test target or diagnostic target,
- a verified kernel R/W bridge if Stage 2 uses one,
- a final UID/GID/capability change only after the above evidence,
- PID 1 parking instead of exiting and causing `Attempted to kill init!`.

The current logs satisfy only the first part of Stage 1: the PI-cycle reaches `EDEADLK`.
After the diagnostic release update, the logs also satisfy the reference-style order requirement that stamping happens before the consumer walk. They still do not prove the Stage 1 to Stage 2 handoff because there is no marker/write proof.
