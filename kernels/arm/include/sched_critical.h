#ifndef SCHED_CRITICAL_H
#define SCHED_CRITICAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __NVIC_PRIO_BITS 4U
#define SCHED_MAX_SYSCALL_PRIO (3U << (8U - __NVIC_PRIO_BITS))

static inline uint32_t sched_critical_enter(void) {
  uint32_t old_basepri;
  __asm volatile("mrs %0, basepri" : "=r"(old_basepri));
  __asm volatile("msr basepri, %0" ::"r"(SCHED_MAX_SYSCALL_PRIO) : "memory");
  __asm volatile("dsb");
  __asm volatile("isb");
  return old_basepri;
}

static inline void sched_critical_exit(uint32_t old_basepri) {
  __asm volatile("msr basepri, %0" ::"r"(old_basepri) : "memory");
  __asm volatile("isb");
}

#ifdef __cplusplus
}
#endif

#endif
