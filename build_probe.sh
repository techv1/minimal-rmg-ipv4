#!/bin/bash
set -euo pipefail

WORKDIR="/root/s/minimal-rmg"
OUTDIR="/root/s"
NDK_CC="/root/android-ndk-r28/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android35-clang"
BUILD_TMP="$(mktemp -d /tmp/x216b-probe.XXXXXX)"
trap 'rm -rf "$BUILD_TMP"' EXIT
PROBE_FIELD="${PROBE_FIELD:-lock}"

case "$PROBE_FIELD" in
    task) PROBE_DEFINE="-DX216B_TASK_PROBE" ;;
    lock) PROBE_DEFINE="-DX216B_LOCK_PROBE" ;;
    *) echo "ERROR: PROBE_FIELD must be task or lock" >&2; exit 2 ;;
esac

cd "$WORKDIR"
if [ ! -x "$NDK_CC" ]; then
    echo "ERROR: Android NDK compiler not found: $NDK_CC" >&2
    exit 1
fi

"$NDK_CC" -static -Os -g0 -fno-unwind-tables -fno-asynchronous-unwind-tables \
    -ffunction-sections -fdata-sections "$PROBE_DEFINE" -I. \
    kaslr.c futex_race.c spray.c pipe_rw.c umh_root.c main.c \
    -o "$BUILD_TMP/init" -Wl,--gc-sections -s

mkdir -p "$BUILD_TMP/rootfs/dev" "$BUILD_TMP/rootfs/proc" "$BUILD_TMP/rootfs/sys"
cp "$BUILD_TMP/init" "$BUILD_TMP/rootfs/init"
chmod 755 "$BUILD_TMP/rootfs/init"
(
    cd "$BUILD_TMP/rootfs"
    find . -print0 | cpio --null -o -H newc > "$BUILD_TMP/initramfs.cpio"
)

cp "$BUILD_TMP/init" "$OUTDIR/init-probe-$PROBE_FIELD"
cp "$BUILD_TMP/initramfs.cpio" "$OUTDIR/initramfs-probe-$PROBE_FIELD.cpio"
echo "Probe artifacts: $OUTDIR/init-probe-$PROBE_FIELD and $OUTDIR/initramfs-probe-$PROBE_FIELD.cpio"
sha256sum "$OUTDIR/init-probe-$PROBE_FIELD" "$OUTDIR/initramfs-probe-$PROBE_FIELD.cpio"
