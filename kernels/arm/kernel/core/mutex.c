#include "mutex.h"
#include "sched_critical.h"
#include "scheduler.h"
#include <stddef.h>
#include <stdint.h>

void mutex_init(mutex_t *m) { m->task = NULL; }

static int mutex_try_acquire(mutex_t *m) {
  uint32_t result;
  uint32_t expexted = 0U;

  // ldrex result, [&m->owner]
  __asm volatile("ldrex %0, [%1]" : "=r"(result) : "r"(&m->task));

  if (result != expexted) {
    __asm volatile("clrex");
    return 0;
  }

  uint32_t store_result;
  // strex store_result, g_current_task, [&m->owner]
  __asm volatile("strex %0, %2, [%1]"
                 : "=r"(store_result)
                 : "r"(&m->task), "r"(g_current_task)
                 : "memory");

  __asm volatile("dmb");

  return (store_result == 0U) ? 1 : 0;
}

void mutex_lock(mutex_t *m) {
    if (m->task == g_current_task) {
        return;
    }
    while (!mutex_try_acquire(m)) {
        sched_delay_ms(1U);
    }
}

void mutex_unlock(mutex_t *m) {
  if (m->task != g_current_task) {
    return;
  }

  __asm volatile("dmb");
  m->task = NULL;
  __asm volatile("dsb");
}

int mutex_trylock(mutex_t *m) {
  if (m->task == g_current_task) {
    return 0;
  }
  return mutex_try_acquire(m);
}
