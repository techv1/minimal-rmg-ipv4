import gdb
import sys

def log(msg):
    print(f"[GDB] {msg}", flush=True)

# ── Kernel addresses ──────────────────────────────────────────────────────────
ADDR_LOCK_CHECK  = 0xffffffc01037543c  # lock check in rt_mutex_adjust_prio_chain
ADDR_RB_ERASE   = 0xffffffc011638ae8  # rb_erase entry

# Our ghost objects — did the walk reach them?
ADDR_FAKE_LOCK   = 0xffffffc012c1fc00  # g_k_fake_lock
ADDR_FAKE_LOCK2  = 0xffffffc012c1fc80  # g_k_fake_lock2
ADDR_GHOST_TASK  = 0xffffffc012c20a00  # g_k_ghost_task

# fake_lock region to dump
ADDR_FAKE_LOCK_REGION = 0xffffffc012c1fc00

lock_check_hits = 0
rb_erase_hits   = 0
ghost_hits      = 0   # LOCK-CHK where x23 == ADDR_FAKE_LOCK or ADDR_FAKE_LOCK2

def dump_region(addr, label):
    log(f"[MEM-DUMP] {label} @ 0x{addr:016x}")
    try:
        result = gdb.execute(f"x/32gx 0x{addr:016x}", to_string=True)
        for line in result.strip().splitlines():
            log(f"[MEM]  {line}")
    except Exception as e:
        log(f"[MEM-DUMP] ERROR: {e}")

# ── Breakpoint 1: lock check ───────────────────────────────────────────────────
class LockCheckBreakpoint(gdb.Breakpoint):
    def __init__(self):
        super().__init__(f"*{ADDR_LOCK_CHECK:#x}", gdb.BP_HARDWARE_BREAKPOINT, internal=False)

    def stop(self):
        global lock_check_hits, ghost_hits
        lock_check_hits += 1
        try:
            x23 = int(gdb.parse_and_eval("$x23")) & 0xffffffffffffffff
            x25 = int(gdb.parse_and_eval("$x25")) & 0xffffffffffffffff
            waiter_lock = int(gdb.parse_and_eval(
                f"*(unsigned long long *)({hex(x25 + 0x38)})")) & 0xffffffffffffffff
            matched = (x23 == waiter_lock)

            is_ghost = (x23 in (ADDR_FAKE_LOCK, ADDR_FAKE_LOCK2))
            if is_ghost:
                ghost_hits += 1

            log(f"[LOCK-CHK HIT #{lock_check_hits}]{'  *** GHOST HIT ***' if is_ghost else ''}")
            log(f"  $x25 (waiter):          0x{x25:016x}")
            log(f"  $x23 (lock):            0x{x23:016x}")
            log(f"  *(x25+0x38) waiter->lock: 0x{waiter_lock:016x}")
            log(f"  Match: {'YES' if matched else 'NO — MISMATCH'}")
            log(f"  Is our ghost lock: {'YES' if is_ghost else 'no'}")

            # Dump x25 (waiter node) — 10 qwords
            if is_ghost or lock_check_hits <= 2:
                try:
                    node_dump = gdb.execute(f"x/10gx 0x{x25:016x}", to_string=True)
                    log(f"  [WAITER NODE DUMP]")
                    for l in node_dump.strip().splitlines():
                        log(f"    {l}")
                except Exception:
                    pass

            # On first hit dump fake_lock region
            if lock_check_hits == 1:
                dump_region(ADDR_FAKE_LOCK_REGION, "fake_lock/fake_lock2 region")

        except Exception as e:
            log(f"[LOCK-CHK] ERROR: {e}")
        return False

# ── Breakpoint 2: rb_erase ────────────────────────────────────────────────────
class RbEraseBreakpoint(gdb.Breakpoint):
    def __init__(self):
        super().__init__(f"*{ADDR_RB_ERASE:#x}", gdb.BP_HARDWARE_BREAKPOINT, internal=False)

    def stop(self):
        global rb_erase_hits
        try:
            x30 = int(gdb.parse_and_eval("$x30")) & 0xffffffffffffffff
            # Filter: only care about calls from rt_mutex range
            if not (0xffffffc010374000 <= x30 <= 0xffffffc010377000):
                return False

            rb_erase_hits += 1
            x0  = int(gdb.parse_and_eval("$x0")) & 0xffffffffffffffff  # node
            x1  = int(gdb.parse_and_eval("$x1")) & 0xffffffffffffffff  # root
            sp  = int(gdb.parse_and_eval("$sp")) & 0xffffffffffffffff

            # Read node's rb_parent_color, rb_right, rb_left
            try:
                node_pc    = int(gdb.parse_and_eval(f"*(unsigned long long *)({hex(x0 + 0x00)})")) & 0xffffffffffffffff
                node_right = int(gdb.parse_and_eval(f"*(unsigned long long *)({hex(x0 + 0x08)})")) & 0xffffffffffffffff
                node_left  = int(gdb.parse_and_eval(f"*(unsigned long long *)({hex(x0 + 0x10)})")) & 0xffffffffffffffff
            except Exception:
                node_pc = node_right = node_left = 0

            is_ghost_node = (node_pc  == ADDR_FAKE_LOCK or
                             node_pc  == ADDR_FAKE_LOCK2 or
                             node_right >= 0xffffffc000000000)

            log(f"[RB-ERASE HIT #{rb_erase_hits}] (caller LR: 0x{x30:016x}){'  *** POSSIBLE GHOST ***' if is_ghost_node else ''}")
            log(f"  $x0 (node):              0x{x0:016x}")
            log(f"  $x1 (root):              0x{x1:016x}")
            log(f"  $sp:                     0x{sp:016x}")
            log(f"  node->rb_parent_color:   0x{node_pc:016x}")
            log(f"  node->rb_right (value):  0x{node_right:016x}")
            log(f"  node->rb_left (target):  0x{node_left:016x}")

            if rb_erase_hits == 1:
                dump_region(ADDR_FAKE_LOCK_REGION, "fake_lock region at rb_erase")

        except Exception as e:
            log(f"[RB-ERASE] ERROR: {e}")
        return False

# ── Register breakpoints (only 2 HW BPs — no WALK-ENTRY to avoid limit) ──────
log("Registering breakpoints:")
log(f"  [LOCK-CHK]   0x{ADDR_LOCK_CHECK:x}")
log(f"  [RB-ERASE]   0x{ADDR_RB_ERASE:x}")

LockCheckBreakpoint()
RbEraseBreakpoint()

log("All breakpoints registered. Continuing execution...")
gdb.execute("continue")

# ── Final summary ─────────────────────────────────────────────────────────────
log(f"=== SESSION SUMMARY ===")
log(f"  LOCK-CHK   hits: {lock_check_hits}")
log(f"  RB-ERASE   hits: {rb_erase_hits}")
log(f"  GHOST      hits: {ghost_hits}  (lock-chk where lock == fake_lock/fake_lock2)")
dump_region(ADDR_FAKE_LOCK_REGION, "FINAL fake_lock region dump")
