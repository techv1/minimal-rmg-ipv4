# minimal-rmg-ipv4: Root-My-Galaxy IPv4 Carrier Port for Samsung Galaxy Tab A9+ 5G (SM-X216B)

This repository contains the ported and audited implementation of the Root-My-Galaxy CVE-2026-43499 local privilege escalation exploit for the Samsung Galaxy Tab A9+ 5G (SM-X216B / X216BXXS9DYJ7 / Linux 5.4.249-qgki).

## Directory Structure

```
minimal-rmg-ipv4/
├── docs/            # Technical audits, findings, geometry reports, and porting guides
├── scripts/         # Automated Capstone offset analyzers, GDB verification scripts, diagnosis
├── tests/           # Dedicated test harness & isolation probes (sigreturn, mcast)
├── backups/         # Historical and intermediate implementations for diff analysis
├── references/      # Baseline mappings, code maps, and porting matrices
├── src/             # Modularized carrier source components
├── common.h         # Core kernel structures, constants, and geometry offsets
├── target.h         # Device-specific symbol addresses and struct definitions
├── futex_race.c     # PI futex requeue race condition exploit primitive
├── pipe_rw.c        # Pipe buffer arbitrary physical memory read/write primitive
├── kaslr.c          # Kernel address space layout randomization bypass
├── spray.c          # Kernel heap spraying utilities
├── umh_root.c       # User Mode Helper / privileged execution path
├── main.c           # Orchestration entrypoint
├── build.sh         # Production cross-compilation pipeline script
└── calibrate.sh     # Timing and geometry calibration helper
```

## Confirmed Findings & Device Geometry

- **Target Device**: Samsung Galaxy Tab A9+ 5G (`SM-X216B`, `gta9p`)
- **Firmware / Kernel**: `X216BXXS9DYJ7` / Linux `5.4.249-qgki-31053672-abX216BXXS9DYJ7` (SM6375 / Snapdragon 695)
- **`rt_mutex_waiter` Size**: **80 bytes** (`0x50`), task at `+0x30`, lock at `+0x38`, prio at `+0x40`.
- **MCAST Stack Carrier Offset**: Live empirical delta of 216 bytes (`0xd8`).
- **Defensive Baseline**:
  - `CONFIG_RKP / KDP`: Disabled (`# CONFIG_RKP / KDP not set` in kernel config).
  - `CONFIG_STATIC_USERMODEHELPER=y` with `CONFIG_STATIC_USERMODEHELPER_PATH=""` disables standard UMH path.
  - Sigreturn / direct credential manipulation primitives audited in `docs/SIGRETURN_GEOMETRY_AND_DIAGNOSIS.md`.

## Building

Requires Android NDK r28 or later:
```bash
./build.sh
```
This compiles a static ELF binary (`/root/s/ipv4-test-artifacts/init`) and builds the testing initramfs cpio (`/root/s/ipv4-test-artifacts/initramfs-ipv4.cpio`).

## Verification & Testing

To run the automated dynamic verification in QEMU:
```bash
./scripts/verify_mcast.sh
```
To run the static offset verification suite:
```bash
python3 scripts/verify_all_offsets.py
```
