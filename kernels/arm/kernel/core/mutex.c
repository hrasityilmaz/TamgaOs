#include "mutex.h"
#include "sched_critical.h"
#include "scheduler.h"
#include "task.h"
#include <stddef.h>
#include <stdint.h>

void mutex_init(mutex_t *m) {
  m->task = NULL;
  m->waiters = NULL;
}

// TODO: priority base planning
// can be problem check here !!!!
static void wq_insert(mutex_t *m, task_t *t) {
  t->wait_next = NULL;

  if (m->waiters == NULL || t->priority < m->waiters->priority) {
    t->wait_next = m->waiters;
    m->waiters = t;
    return;
  }

  task_t *cur = m->waiters;
  while (cur->wait_next != NULL &&
         cur->wait_next->priority <= t->priority) //!!!
    cur = cur->wait_next;

  t->wait_next = cur->wait_next;
  cur->wait_next = t;
}

static task_t *wq_pop(mutex_t *m) {
  task_t *t = m->waiters;
  if (t != NULL)
    m->waiters = t->wait_next;
  return t;
}

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

// TODO: !!!!!!!
void mutex_lock(mutex_t *m) {
  // if (m->task == g_current_task) {
  //   return;
  // }
  // while (!mutex_try_acquire(m)) {
  //   sched_delay_ms(1U);
  // }

  for (;;) {
    if (m->task == g_current_task)
      return;
    if (mutex_try_acquire(m))
      return;
    uint32_t p = sched_critical_enter();
    if (m->task == NULL) {
      sched_critical_exit(p);
      continue;
    }
    g_current_task->state = TASK_BLOCKED;
    wq_insert(m, g_current_task);
    sched_critical_exit(p);

    sched_block();   /* must stay BLOCKED, not READY */
  }
}

void mutex_unlock(mutex_t *m) {
  if (m->task != g_current_task) {
    return;
  }

  __asm volatile("dmb");

  uint32_t p = sched_critical_enter();
  task_t *waiter = wq_pop(m);

  if (waiter != NULL) {
    m->task = waiter;
    sched_critical_exit(p);
    __asm volatile("dsb");
    sched_wake_task(waiter);
    return;
  }

  m->task = NULL;
  sched_critical_exit(p);
  __asm volatile("dsb");
}

int mutex_trylock(mutex_t *m) {
  if (m->task == g_current_task)
    return 0;

  return mutex_try_acquire(m);
}