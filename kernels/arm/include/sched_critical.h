#ifndef SCHED_CRITICAL_H
#define SCHED_CRITICAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * MISRA-C:2012 Directive 4.3 — Justified Deviation
 *
 * Rationale: Entering/exiting a critical section on ARMv7-M requires
 * direct access to the PRIMASK special register, which has no
 * standard-C representation. The assembly is isolated to these two
 * small, single-purpose functions (never inlined ad hoc elsewhere in
 * the codebase), satisfying the "encapsulated and isolated" intent
 * of Directive 4.3.
 *
 * sched_critical_enter():
 *   - reads the CURRENT PRIMASK value (i.e. whether interrupts were
 *     already masked by an outer critical section) BEFORE masking
 *   - then sets PRIMASK to mask all maskable interrupts (cpsid i)
 *   - the returned value must be passed unchanged to
 *     sched_critical_exit() to correctly restore the prior state,
 *     making critical sections safely nestable
 */
static inline uint32_t sched_critical_enter(void) {
    uint32_t primask;
    __asm volatile("mrs %0, primask" : "=r"(primask));
    __asm volatile("cpsid i");
    return primask;
}

/*
 * sched_critical_exit():
 *   - restores PRIMASK to the value captured by the matching
 *     sched_critical_enter() call
 */
static inline void sched_critical_exit(uint32_t primask) {
    __asm volatile("msr primask, %0" :: "r"(primask));
}

#ifdef __cplusplus
}
#endif

#endif /* SCHED_CRITICAL_H */