# SM-X216B kernel offset audit

**Target:** Samsung Galaxy Tab A9+ 5G (`SM-X216B` / `gta9p`)  
**Kernel:** `5.4.249-qgki-31053672-abX216BXXS9DYJ7`, AArch64  
**Kernel image:** `/root/s/Image.nokdp`  
**Symbol image:** `/root/s/vmlinux.elf`  
**vmlinux SHA-256:** `dfc0912ee00696bed45f0852d490a75d20d336a16fc87288b155d513bbcea29f`

## Findings

1. The 5.4 target waiter is `0x50` bytes. Its important fields are `task +0x30`, `lock +0x38`, `prio +0x40`, and `deadline +0x48`. The 5.15 reference layout must not be substituted.
2. Static frame arithmetic for this exact `vmlinux` places the stale waiter at `SP_entry - 0x198` and the IPv4 `MCAST_BLOCK_SOURCE` input copy at `SP_entry - 0x2c0`. This leaves a `0x20` gap; the 264-byte copy does not overlap the waiter. Thus neither `0x88` in current `target.h` nor the previously documented `0xc8` is supported by these frame calculations as the MCAST waiter-base offset.
3. The sigreturn geometry requires `0x148` bytes into the kernel restore buffer. Since userspace payload data begins at `vregs`, `0x10` bytes after the FPSIMD record start, the equivalent offset from the record start is `0x158`. Current `futex_race.c` defaults to `0x148` and adds it to the FPSIMD record pointer, which places the payload 16 bytes early by this calculation. Set `SIGRETURN_WAITER_OFF=0x158` (or change the base pointer to `vregs` and retain `0x148`) before treating the sigreturn stamp as correctly aligned.
4. Static placement does not validate the race state or the forged waiter. The latest `/root/s/qemu-gdb-serial.log` has `FUTEX_WAIT_REQUEUE_PI -> -1, errno=11 (EAGAIN)`, then `sigreturn stamp completed status=1`, then the ghost-write respray and a kernel BUG in `rt_mutex_adjust_prio_chain`. The status confirms the userspace handler found an FPSIMD record; it is not runtime proof that the PI chain consumed valid waiter contents.

## Writer selected by the latest run

The current `futex_race.c` selects sigreturn by default and switches to MCAST only when `TRIGGER=mcast` is set. The `-DUSE_MCAST_STAMP` build define in `build.sh` does not select this branch. The latest serial log confirms sigreturn was selected (`entering sigreturn stamp`). The sigreturn calculation below therefore applies to that run; the native MCAST result is an audit of the alternate path.

## Address base and symbol offsets

The link base used by this target is `0xffffffc010080000`. QEMU is run with `nokaslr`; its log reports `Kernel Offset: disabled`. Addresses below are link addresses, so the offset column is `address - 0xffffffc010080000`.

| Symbol | Link address | Offset | Assessment |
|---|---:|---:|---|
| `__arm64_sys_futex` | `0xffffffc0103c3498` | `0x00343498` | Verified in `vmlinux.elf` |
| `do_futex` | `0xffffffc0103c02dc` | `0x003402dc` | Verified in `vmlinux.elf` |
| `rt_mutex_adjust_prio_chain` | `0xffffffc0103753bc` | `0x002f53bc` | Verified in `vmlinux.elf` |
| `__arm64_sys_setsockopt` | `0xffffffc0112ebbd0` | `0x0126bbd0` | Verified in `vmlinux.elf` |
| `do_ip_setsockopt` | `0xffffffc0113b5d88` | `0x01335d88` | Verified in `vmlinux.elf` |
| `__arm64_sys_rt_sigreturn` | `0xffffffc0102cde24` | `0x0024de24` | Verified in `vmlinux.elf` |
| `ashmem_fops` | `0xffffffc0121cee28` | `0x0214ee28` | Matches `target.h` |
| `configfs_write_bin_file.cfi_jt` | `0xffffffc01167595c` | `0x015f595c` | Matches `target.h` |
| `configfs_read_bin_file.cfi_jt` | `0xffffffc0116738d8` | `0x015f38d8` | Matches `target.h` |
| `anon_pipe_buf_ops` | `0xffffffc0121f55b8` | `0x021755b8` | Matches `target.h` |
| `init_task` | `0xffffffc0129e0280` | `0x02960280` | Matches `target.h` |
| `system_unbound_wq` | `0xffffffc01296f9c8` | `0x028ef9c8` | Matches `target.h` |
| `call_usermodehelper_exec_work` | `0xffffffc010317274` | `0x00297274` | Matches `target.h` |
| `call_usermodehelper_exec_work.cfi_jt` | `0xffffffc011681870` | `0x01601870` | Matches `target.h` |
| `selinux_state` | `0xffffffc012b9f000` | `0x02b1f000` | Matches `target.h` |

These values are valid for this image with KASLR disabled. A boot with KASLR enabled needs the runtime slide added to symbol addresses.

## Waiter layout and `do_futex` placement

The optimized target disassembly at `do_futex` (`0xffffffc0103c02dc`) shows a `0x60`-byte register/frame-pointer save, followed by `sub sp, sp, #0x1b0`. At the `FUTEX_WAIT_REQUEUE_PI` path (`0xffffffc0103c0a64`), it passes `x29 - 0xc8` to `rt_mutex_init_waiter`.

Relative to the entry SP of `__arm64_sys_futex`:

```text
__arm64_sys_futex frame:   0x70
do_futex saved frame:      0x60
waiter local from x29:     0xc8
waiter base depth:         0x70 + 0x60 + 0xc8 = 0x198
waiter address:            SP_entry - 0x198
waiter end:                SP_entry - 0x148   (0x50-byte waiter)
task field:                SP_entry - 0x168   (waiter + 0x30)
lock field:                SP_entry - 0x160   (waiter + 0x38)
```

The current target structure definitions use an 80-byte waiter with `task +0x30`, `lock +0x38`, `prio +0x40`, and `deadline +0x48`. This matches the Linux 5.4 profile in `target.h`; the iQOO and GhostLockAdapt profiles describe different 5.15 kernels.

## Native IPv4 MCAST stack calculation

For `MCAST_BLOCK_SOURCE` (`optname 43`), this image's `do_ip_setsockopt` code clears and copies `0x108` bytes at `sp + 0x20` (the copy completes at `0xffffffc0113b617c`). The stack frames on the call path are visible in the exact `vmlinux.elf`:

| Frame | Stack allocation |
|---|---:|
| `__arm64_sys_setsockopt` | `0x10` |
| `__sys_setsockopt` | `0x70` |
| `sock_common_setsockopt` | `0x40` |
| `ip_setsockopt` | `0x30` |
| `do_ip_setsockopt` | `0x190` |
| Copy begins above `do_ip_setsockopt` SP | `-0x20` depth adjustment |

Therefore, relative to the entry SP of `__arm64_sys_setsockopt`, the copy begins at depth:

```text
0x10 + 0x70 + 0x40 + 0x30 + 0x190 - 0x20 = 0x2c0
```

The copied byte interval is `[SP_entry - 0x2c0, SP_entry - 0x1b8)`. The stale waiter interval is `[SP_entry - 0x198, SP_entry - 0x148)`. The copy ends `0x20` below the waiter start in address space, so this native MCAST copy misses the waiter under the static frame model.

This conflicts with older local notes that state a `0xc8` overlap, and with the current header's `MCAST_64_WAITER_BASE_OFF=0x88`. The discrepancy is material: the older notes do not include all frame sizes consistently. Existing GDB logs show the IPv4 copy at `do_ip_setsockopt`'s `sp+0x20`, but do not provide a clean paired capture of the stale waiter address from the same run. Treat the no-overlap result as a static calculation that needs one paired GDB check before finalizing any MCAST offset.

## Sigreturn stack calculation

The exact binary's restore path has these frames:

| Frame | Stack allocation |
|---|---:|
| `__arm64_sys_rt_sigreturn` | `0x30` |
| `restore_sigframe` | `0x60` |
| `restore_fpsimd_context` saved registers | `0x30` |
| `restore_fpsimd_context` local restore buffer | `0x220` |
| **Total to restore buffer** | **`0x2e0`** |

`restore_fpsimd_context` zeroes its local buffer and copies the FPSIMD data into it. Relative to the sigreturn syscall entry, the local buffer starts at `SP_entry - 0x2e0`. The same thread's stale waiter location from its futex syscall is at `SP_entry - 0x198`; this gives a `0x148`-byte offset into the restore buffer. Because `vregs` starts `0x10` bytes into the userspace FPSIMD record, the fake waiter payload must begin at record offset `0x158`. Current code uses `dst = (unsigned char *)fpsimd + off`, so its default `off=0x148` is record-relative and is 16 bytes early; `0x148` is correct only if added to the `vregs` pointer.

This is the strongest stack-placement calculation available from the local binary. The latest QEMU run still panics during the subsequent PI-chain operation, so correct geometric placement is not yet proof that the waiter topology, lock, or field values are valid.

## Runtime failure boundary

The latest serial log establishes this sequence:

```text
CMP_REQUEUE_PI -> EDEADLK
FUTEX_WAIT_REQUEUE_PI -> EAGAIN
sigreturn stamp completed status=1
phase2 ghost_write -> ashmem_fops.write_iter
kernel BUG at kernel/locking/rtmutex_common.h:60
rt_mutex_adjust_prio_chain -> fatal exception / panic
```

The reference iQOO waiter path accepts its expected `ETIMEDOUT`, unlocks the chain, and waits for the owner handoff before invoking its writer. This run's `EAGAIN` is a different path and should not be counted as equivalent evidence that the waiter was queued/requeued as intended. The panic is in the PI-chain consumer, so the serial log does not establish that the requested target overwrite occurred.

## Confidence and remaining checks

| Result | Confidence |
|---|---|
| Kernel symbol addresses and target base | High; symbol table in the matching-version `vmlinux.elf` |
| 5.4 waiter size/field offsets and `do_futex` local | High; target binary disassembly and target-specific notes |
| Sigreturn restore-buffer overlap and `0x158` record-relative source offset | High static confidence; exact restore-path prologues plus local copy geometry. Current source default `0x148` is 16 bytes early when based at the record start. |
| Native IPv4 MCAST waiter overlap | Not confirmed; exact frame calculation predicts a 32-byte miss, contradicting older notes and not yet paired with a same-run waiter-address capture |
| Valid PI-chain state / successful controlled write | Not established; latest QEMU run returns `EAGAIN` then panics in the PI walk |

No source offsets were changed as part of this report.
