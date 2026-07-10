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

/* Critical section based acquire — FPU safe */
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
    g_current_task->state = TASK_BLOCKED;
    wq_insert(m, g_current_task);
    sched_block_locked();        /* state+queue+next-seçim+PendSV trigger */
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