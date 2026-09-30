#include "common.h"

int spray_waiter(struct spray_plan *plan) {
    (void)plan;
    return -1;
}

uint64_t spray_warmup_marker(void) {
    return 0xDEADBEEFCAFEBABEULL;
}
