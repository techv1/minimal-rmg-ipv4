import gdb
import sys

def log(msg):
    print(f"{msg}", flush=True)

class RestoreFpsimdBP(gdb.Breakpoint):
    def __init__(self):
        super().__init__("*0xffffffc0102cf86c", gdb.BP_HARDWARE_BREAKPOINT, internal=False)

    def stop(self):
        log("=== RESTORE-FPSIMD ===")
        try:
            x0 = int(gdb.parse_and_eval("$x0")) & 0xffffffffffffffff
            log(f"struct base (x0) = 0x{x0:x}")
            for off in [0x148, 0x158, 0x138]:
                try:
                    v1 = int(gdb.parse_and_eval(f"*(void**)({x0+off:#x})")) & 0xffffffffffffffff
                    v2 = int(gdb.parse_and_eval(f"*(void**)({x0+off+8:#x})")) & 0xffffffffffffffff
                    log(f"mem at x0+0x{off:x}  = 0x{v1:016x} 0x{v2:016x}")
                except Exception as e:
                    log(f"mem at x0+0x{off:x}  = (unreadable user address 0x{x0+off:x})")
        except Exception as e:
            log(f"Error in RestoreFpsimdBP: {e}")
        
        walk_bp.enabled = True
        rberase_bp.enabled = True
        return False

class WalkBP(gdb.Breakpoint):
    def __init__(self):
        super().__init__("*0xffffffc010375424", gdb.BP_HARDWARE_BREAKPOINT, internal=False)
        self.enabled = False

    def stop(self):
        log("=== WALK ===")
        try:
            x25 = int(gdb.parse_and_eval("$x25")) & 0xffffffffffffffff
            log(f"waiter x25 = 0x{x25:x}")
            for off in [0x00, 0x08, 0x10, 0x30, 0x38]:
                try:
                    val = int(gdb.parse_and_eval(f"*(void**)({x25+off:#x})")) & 0xffffffffffffffff
                    log(f"x25+0x{off:02x} = 0x{val:016x}")
                except Exception as e:
                    log(f"x25+0x{off:02x} = read error: {e}")
        except Exception as e:
            log(f"Error in WalkBP: {e}")
        return False

class RbEraseBP(gdb.Breakpoint):
    def __init__(self):
        super().__init__("*0xffffffc011638ae8", gdb.BP_HARDWARE_BREAKPOINT, internal=False)
        self.enabled = False

    def stop(self):
        log("=== RB-ERASE ===")
        try:
            x0 = int(gdb.parse_and_eval("$x0")) & 0xffffffffffffffff
            x8 = int(gdb.parse_and_eval("$x8")) & 0xffffffffffffffff
            x9 = int(gdb.parse_and_eval("$x9")) & 0xffffffffffffffff
            log(f"x0=0x{x0:x} x8=0x{x8:x} x9=0x{x9:x}")
            try:
                n0 = int(gdb.parse_and_eval(f"*(void**)({x0:#x})")) & 0xffffffffffffffff
                n8 = int(gdb.parse_and_eval(f"*(void**)({x0+8:#x})")) & 0xffffffffffffffff
                n16 = int(gdb.parse_and_eval(f"*(void**)({x0+16:#x})")) & 0xffffffffffffffff
                log(f"node[0]=0x{n0:016x} node[8]=0x{n8:016x} node[16]=0x{n16:016x}")
            except Exception as e:
                log(f"node read error: {e}")
        except Exception as e:
            log(f"Error in RbEraseBP: {e}")
        return False

class PanicBP(gdb.Breakpoint):
    def __init__(self):
        super().__init__("panic", gdb.BP_BREAKPOINT, internal=False)

    def stop(self):
        log("\n=== PANIC REACHED ===")
        return True

restore_bp = RestoreFpsimdBP()
walk_bp = WalkBP()
rberase_bp = RbEraseBP()
panic_bp = PanicBP()

log("Breakpoints registered. Continuing execution...")
