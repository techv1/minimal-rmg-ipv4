#!/usr/bin/env bash
set -euo pipefail

# Benign MCAST verification harness
# Tests the ordinary MCAST kernel path without exploitation

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_ROOT="/root/s"
LOG_DIR="$WORKSPACE_ROOT/logs"
SERIAL_LOG="$LOG_DIR/qemu-benign-mcast-serial.log"

mkdir -p "$LOG_DIR"

echo "======================================================================="
echo "[*] Benign MCAST Path Verification"
echo "======================================================================="

# Clean up
pkill -9 -f "qemu-system-aarch64" 2>/dev/null || true
sleep 1

# Build benign MCAST binary
echo "[+] Building benign MCAST binary..."
/root/android-ndk-r28/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android35-clang \
    -static -Os -g0 -I. benign_mcast.c \
    -o "$WORKSPACE_ROOT/benign_mcast" -s

if [ ! -f "$WORKSPACE_ROOT/benign_mcast" ]; then
    echo "[-] Failed to build benign_mcast"
    exit 1
fi

# Copy to initramfs
WORKDIR=$(mktemp -d)
mkdir -p "$WORKDIR/rootfs"
cp -r "$WORKSPACE_ROOT/ipv4-test-artifacts/rootfs/." "$WORKDIR/rootfs/"
cp "$WORKSPACE_ROOT/benign_mcast" "$WORKDIR/rootfs/bin/benign_mcast"
chmod 755 "$WORKDIR/rootfs/bin/benign_mcast"

# Create simple script to run benign_mcast
cat > "$WORKDIR/rootfs/init" << 'INIT_EOF'
#!/bin/init
set -e

echo "=========================================="
echo "  Benign MCAST Path Verification"
echo "=========================================="

# Mount filesystems
mkdir -p /dev /proc /sys /bin
mount -t devtmpfs devtmpfs /dev
mount -t proc proc /proc
mount -t sysfs sysfs /sys

# Run benign MCAST test
echo "[*] Running benign MCAST test..."
/bin/benign_mcast
echo "[*] Benign MCAST test complete"
echo "[*] Verification successful - MCAST path is working"

# Loop forever
while true; do sleep 100; done
INIT_EOF
chmod +x "$WORKDIR/rootfs/init"

# Build new initramfs
(
    cd "$WORKDIR/rootfs"
    find . -print0 | cpio --null -o -H newc > "$WORKSPACE_ROOT/initramfs-benign.cpio"
)

echo "[+] Testing with benign MCAST binary..."
echo "[+] Press Ctrl+C to stop verification"
echo ""

# Run QEMU with benign initramfs
qemu-system-aarch64 \
    -M virt,gic-version=3 \
    -cpu cortex-a76 \
    -smp 4 \
    -m 4096 \
    -kernel "$WORKSPACE_ROOT/Image.nokdp" \
    -initrd "$WORKSPACE_ROOT/initramfs-benign.cpio" \
    -nographic \
    -append "console=ttyAMA0 earlycon=pl011,0x09000000 nokaslr panic=1 rdinit=/init" \
    -s > "$SERIAL_LOG" 2>&1 &
QEMU_PID=$!

sleep 15

echo ""
echo "[+] Serial log saved to: $SERIAL_LOG"
echo "[+] GDB connection available at: localhost:1234"
echo "[+] Running: ps -p $QEMU_PID"
ps -p $QEMU_PID 2>/dev/null || echo "QEMU terminated"

# Cleanup
kill -9 $QEMU_PID 2>/dev/null || true

echo "[+] Verification complete"
