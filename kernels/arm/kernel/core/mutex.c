#include "mutex.h"
#include "sched_critical.h"
#include "scheduler.h"
#include "task.h"
#include <stddef.h>
#include <stdint.h>
#include "uart.h" 

void mutex_init(mutex_t *m) {
  m->task = NULL;
  m->waiters = NULL;
  m->owner_original_priority = 0U;
  m->is_elevated = 0U;
}

static void wq_insert(mutex_t *m, task_t *t) {
  t->wait_next = NULL;
  if (m->waiters == NULL || t->priority < m->waiters->priority) {
    t->wait_next = m->waiters;
    m->waiters = t;
    return;
  }
  task_t *cur = m->waiters;
  while (cur->wait_next != NULL &&
         cur->wait_next->priority <= t->priority)
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
  uint32_t p = sched_critical_enter();
  int result = 0;
  if (m->task == NULL) {
    m->task = g_current_task;
    result = 1;
  }
  sched_critical_exit(p);
  return result;
}

void mutex_lock(mutex_t *m) {
  for (;;) {
    if (mutex_try_acquire(m)) return;

    uint32_t p = sched_critical_enter();
    if (m->task == NULL) {
      sched_critical_exit(p);
      continue;
    }

    /* When HIGH task blocked, LOW owner's prioritywill go up.
     * is_elevated flag we are saving only needed
    */
    if (g_current_task->priority < m->task->priority) {
      if (!m->is_elevated) {
        m->owner_original_priority = m->task->priority;
        m->is_elevated = 1U;
      }
      m->task->priority = g_current_task->priority;
      uart_puts("[PI] ELEVATED\r\n");
    }

    g_current_task->state = TASK_BLOCKED;
    wq_insert(m, g_current_task);
    sched_block_locked();
    sched_critical_exit(p);
    __asm volatile("dsb");
    __asm volatile("isb");
  }
}

void mutex_unlock(mutex_t *m) {
  if (m->task != g_current_task)
    return;

  __asm volatile("dmb");

  uint32_t p = sched_critical_enter();
  task_t *waiter = wq_pop(m);
  /* New added priority based!! */
  if (m->is_elevated) {
    m->task->priority = m->owner_original_priority;
    m->is_elevated = 0U;
    uart_puts("[PI] RESTORED\r\n");
  }

  m->task = NULL;

  __asm volatile("dsb");
  if (waiter != NULL) {
    sched_wake_task(waiter);
  }
  sched_critical_exit(p);
}

int mutex_trylock(mutex_t *m) {
  return mutex_try_acquire(m);
}