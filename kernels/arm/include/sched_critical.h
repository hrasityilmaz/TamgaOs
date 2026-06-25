#ifndef SCHED_CRITICAL_H
#define SCHED_CRITICAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline uint32_t sched_critical_enter(void) {
    uint32_t primask;
    __asm volatile("mrs %0, primask" : "=r"(primask));
    __asm volatile("cpsid i");
    return primask;
}

static inline void sched_critical_exit(uint32_t primask) {
    __asm volatile("msr primask, %0" :: "r"(primask));
}

#ifdef __cplusplus
}
#endif

#endif