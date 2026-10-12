#!/usr/bin/env bash
set -euo pipefail

# ==============================================================================
# verify_mcast.sh - Automated Dynamic MCAST Geometry Verification via QEMU & GDB
# Target: Samsung Galaxy Tab A9+ 5G (SM-X216B / X216BXXS9DYJ7 / Linux 5.4.249)
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_ROOT="/root/s"
LOG_DIR="$WORKSPACE_ROOT/logs"
SERIAL_LOG="$LOG_DIR/qemu-mcast-verify-serial.log"
GDB_LOG="$LOG_DIR/qemu-mcast-verify-gdb.log"

mkdir -p "$LOG_DIR"

echo "======================================================================="
echo "[*] SM-X216B Dynamic MCAST Geometry Verification Pipeline"
echo "======================================================================="

# 1. Clean up any existing instances
echo "[+] Step 1: Cleaning up existing QEMU and GDB processes..."
pkill -9 -f "qemu-system-aarch64" 2>/dev/null || true
pkill -9 -f "gdb-multiarch" 2>/dev/null || true
sleep 1

# 2. Check prerequisites
KERNEL_IMG="$WORKSPACE_ROOT/Image.nokdp"
INITRAMFS_IMG="$WORKSPACE_ROOT/ipv4-test-artifacts/initramfs-ipv4.cpio"
VMLINUX_ELF="$WORKSPACE_ROOT/vmlinux.elf"
GDB_SCRIPT="$SCRIPT_DIR/verify_mcast_gdb.py"

for f in "$KERNEL_IMG" "$INITRAMFS_IMG" "$VMLINUX_ELF" "$GDB_SCRIPT"; do
    if [ ! -f "$f" ]; then
        echo "[-] ERROR: Required artifact not found: $f"
        exit 1
    fi
done

# 3. Launch QEMU with GDB stub (-s = tcp::1234) in the background
echo "[+] Step 2: Spawning QEMU with GDB stub on localhost:1234..."
qemu-system-aarch64 \
    -M virt,gic-version=3 \
    -cpu cortex-a76 \
    -smp 4 \
    -m 4096 \
    -kernel "$KERNEL_IMG" \
    -initrd "$INITRAMFS_IMG" \
    -nographic \
    -append "console=ttyAMA0 earlycon=pl011,0x09000000 nokaslr panic=1 rdinit=/init initcall_blacklist=scm_mem_protection_init,qcom_scm_init,scm_mem_protection_init_do,fastrpc_init,adsprpc_init,msm_geni_serial_init,ehci_pci_init,cryptomgr_schedule_test,cryptomgr_init,cryptd_init,algif_init,crypto_algapi_init,audit_init,serio_init,keyboard_init,mousedev_init,phx_klog_bootstage_init" \
    -s \
    > "$SERIAL_LOG" 2>&1 &

QEMU_PID=$!
echo "[+] QEMU running in background (PID: $QEMU_PID)"

# 4. Wait for GDB stub socket to open
echo "[+] Step 3: Waiting for QEMU GDB stub on port 1234..."
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
echo "[+] QEMU GDB stub is ready!"

# 5. Run GDB with verification script
echo "[+] Step 4: Attaching GDB and running verify_mcast_gdb.py..."
set +e
timeout 600s gdb-multiarch -batch \
    -ex "file $VMLINUX_ELF" \
    -ex "target remote localhost:1234" \
    -x "$GDB_SCRIPT" \
    -ex "quit" \
    > "$GDB_LOG" 2>&1
GDB_STATUS=$?
set -e

# 6. Stop QEMU
echo "[+] Step 5: Terminating QEMU (PID: $QEMU_PID)..."
kill -9 "$QEMU_PID" 2>/dev/null || true
wait "$QEMU_PID" 2>/dev/null || true

# 7. Analyze verification output
echo "======================================================================="
echo "[*] Verification Session Results"
echo "======================================================================="

if [ -f "$GDB_LOG" ]; then
    echo "--- [ GDB Trace Highlights ] ---"
    grep -E "BREAKPOINT HIT|DYNAMIC GEOMETRY|MATCH CONFIRMED|MISMATCH|Inspecting stamped|Fake Waiter Region|Setting w20" "$GDB_LOG" || cat "$GDB_LOG" | tail -n 40
    echo "--------------------------------"
fi

if grep -q "MATCH CONFIRMED" "$GDB_LOG"; then
    echo "[SUCCESS] Dynamic MCAST geometry verified! MCAST_WAITER_OFF=0xc8 matches live kernel execution."
elif grep -q "BREAKPOINT HIT" "$GDB_LOG"; then
    echo "[INFO] Breakpoints hit. Review details in $GDB_LOG and $SERIAL_LOG."
else
    echo "[WARNING] Verification did not hit expected breakpoints. Check logs:"
    echo "  GDB log:    $GDB_LOG"
    echo "  Serial log: $SERIAL_LOG"
fi

echo "======================================================================="
