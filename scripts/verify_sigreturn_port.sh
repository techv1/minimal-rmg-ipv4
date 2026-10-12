#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_ROOT="/root/s"
LOG_DIR="$WORKSPACE_ROOT/logs"
SERIAL_LOG="$LOG_DIR/qemu-sigret-port-serial.log"
GDB_LOG="$LOG_DIR/qemu-sigret-port-gdb.log"

mkdir -p "$LOG_DIR"

echo "======================================================================="
echo "[*] Sigreturn Port Dynamic Verification Harness"
echo "======================================================================="

pkill -9 -f "qemu-system-aarch64" 2>/dev/null || true
pkill -9 -f "gdb-multiarch" 2>/dev/null || true
sleep 1

echo "[+] Compiling test_sigreturn_port..."
/root/android-ndk-r28/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android35-clang \
    -static -O2 -g -pthread "$SCRIPT_DIR/test_sigreturn_port.c" \
    -o "$WORKSPACE_ROOT/test_sigreturn_port"

echo "[+] Building test initramfs..."
WORKDIR=$(mktemp -d)
mkdir -p "$WORKDIR/rootfs"
cp -r "$WORKSPACE_ROOT/ipv4-test-artifacts/rootfs/." "$WORKDIR/rootfs/"
cp "$WORKSPACE_ROOT/test_sigreturn_port" "$WORKDIR/rootfs/init"
chmod 755 "$WORKDIR/rootfs/init"

(
    cd "$WORKDIR/rootfs"
    find . -print0 | cpio --null -o -H newc > "$WORKSPACE_ROOT/initramfs-sigret.cpio"
)
rm -rf "$WORKDIR"

# Launch QEMU with matching initcall_blacklist
echo "[+] Starting QEMU on port 1234..."
qemu-system-aarch64 \
    -M virt,gic-version=3 \
    -cpu cortex-a76 \
    -smp 4 \
    -m 4096 \
    -kernel "$WORKSPACE_ROOT/Image.nokdp" \
    -initrd "$WORKSPACE_ROOT/initramfs-sigret.cpio" \
    -nographic \
    -append "console=ttyAMA0 earlycon=pl011,0x09000000 nokaslr panic=1 rdinit=/init initcall_blacklist=scm_mem_protection_init,qcom_scm_init,scm_mem_protection_init_do,fastrpc_init,adsprpc_init,msm_geni_serial_init,ehci_pci_init,cryptomgr_schedule_test,algif_init,crypto_algapi_init,audit_init,serio_init,keyboard_init,mousedev_init,phx_klog_bootstage_init" \
    -s > "$SERIAL_LOG" 2>&1 &
QEMU_PID=$!

echo "[+] Waiting for QEMU GDB stub..."
MAX_WAIT=20
WAIT_COUNT=0
while ! nc -z 127.0.0.1 1234 2>/dev/null; do
    sleep 0.5
    WAIT_COUNT=$((WAIT_COUNT + 1))
    if [ $WAIT_COUNT -ge $((MAX_WAIT * 2)) ]; then
        echo "[-] ERROR: Timed out waiting for QEMU GDB stub."
        kill -9 "$QEMU_PID" 2>/dev/null || true
        exit 1
    fi
done
echo "[+] QEMU GDB stub ready!"

echo "[+] Attaching GDB..."
set +e
timeout 60s gdb-multiarch -batch \
    -ex "file $WORKSPACE_ROOT/vmlinux.elf" \
    -ex "target remote localhost:1234" \
    -x "$SCRIPT_DIR/verify_sigreturn_port_gdb.py" \
    -ex "quit" \
    > "$GDB_LOG" 2>&1
GDB_STATUS=$?
set -e

kill -9 "$QEMU_PID" 2>/dev/null || true
wait "$QEMU_PID" 2>/dev/null || true

echo "======================================================================="
echo "[*] SERIAL LOG OUTPUT:"
echo "======================================================================="
cat "$SERIAL_LOG" | tail -n 25 || true

echo "======================================================================="
echo "[*] GDB LOG OUTPUT:"
echo "======================================================================="
cat "$GDB_LOG" || true
