#!/bin/bash
set -euo pipefail

WORKDIR="/root/s/minimal-rmg-ipv4"
OUTDIR="/root/s/ipv4-test-artifacts"
NDK_CC="/root/android-ndk-r28/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android35-clang"
BUILD_TMP="$(mktemp -d /tmp/x216b-build.XXXXXX)"
trap 'rm -rf "$BUILD_TMP"' EXIT

cd "$WORKDIR"

if [ ! -x "$NDK_CC" ]; then
    echo "ERROR: Android NDK compiler not found: $NDK_CC" >&2
    exit 1
fi

echo "Compiling current minimal-rmg sources..."
"$NDK_CC" -static -Os -g0 -fno-unwind-tables -fno-asynchronous-unwind-tables \
    -ffunction-sections -fdata-sections -I. \
    -DUSE_MCAST_STAMP kaslr.c futex_race.c spray.c pipe_rw.c umh_root.c main.c \
    -o "$BUILD_TMP/init" -Wl,--gc-sections -s

mkdir -p "$BUILD_TMP/rootfs/dev" "$BUILD_TMP/rootfs/proc" "$BUILD_TMP/rootfs/sys"
cp "$BUILD_TMP/init" "$BUILD_TMP/rootfs/init"
chmod 755 "$BUILD_TMP/rootfs/init"

echo "Packaging a fresh initramfs..."
(
    cd "$BUILD_TMP/rootfs"
    find . -print0 | cpio --null -o -H newc > "$BUILD_TMP/initramfs-ipv4.cpio"
)

cp "$BUILD_TMP/init" "$OUTDIR/init.new"
mv "$OUTDIR/init.new" "$OUTDIR/init"
cp "$BUILD_TMP/initramfs-ipv4.cpio" "$OUTDIR/initramfs-ipv4.cpio.new"
mv "$OUTDIR/initramfs-ipv4.cpio.new" "$OUTDIR/initramfs-ipv4.cpio"

echo "Done. Refreshed $OUTDIR/init and $OUTDIR/initramfs-ipv4.cpio"
ls -lh "$OUTDIR/init" "$OUTDIR/initramfs-ipv4.cpio"
