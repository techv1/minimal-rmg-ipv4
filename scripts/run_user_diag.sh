#!/usr/bin/env bash
set -euo pipefail

WORKSPACE_ROOT="/root/s"
SERIAL_LOG="/tmp/qemu-user-diag-serial.log"
GDB_LOG="/tmp/qemu-user-diag-gdb.log"

pkill -9 -f "qemu-system-aarch64" 2>/dev/null || true
pkill -9 -f "gdb-multiarch" 2>/dev/null || true
sleep 1

KERNEL_IMG="$WORKSPACE_ROOT/Image.nokdp"
INITRAMFS_IMG="$WORKSPACE_ROOT/ipv4-test-artifacts/initramfs-ipv4.cpio"
VMLINUX_ELF="$WORKSPACE_ROOT/vmlinux.elf"
GDB_SCRIPT="$WORKSPACE_ROOT/minimal-rmg-ipv4/run_user_diag.gdb"

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

timeout 180s gdb-multiarch -batch \
    -x "$GDB_SCRIPT" \
    > "$GDB_LOG" 2>&1 || true

kill -9 "$QEMU_PID" 2>/dev/null || true
wait "$QEMU_PID" 2>/dev/null || true
