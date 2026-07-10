#include "sched_critical.h"
#include "scheduler.h"
#include "semaphore.h"
#include "stddef.h"
#include "task.h"
#include "uart.h"
#include <stdint.h>

void sem_init(sem_t *s, int32_t initial, int32_t max) {
  s->count = initial;
  s->max = max;
  s->waiters = NULL;
}

static task_t *wq_pop(sem_t *s) {
  task_t *t = s->waiters;
  if (t != NULL)
    s->waiters = t->wait_next;
  return t;
}

static void wq_insert(sem_t *s, task_t *t) {
  t->wait_next = NULL;
  if (s->waiters == NULL || t->priority < s->waiters->priority) {
    t->wait_next = s->waiters;
    s->waiters = t;
    return;
  }

  task_t *cur = s->waiters;
  while (cur->wait_next != NULL && cur->wait_next->priority <= t->priority)
    cur = cur->wait_next;

  t->wait_next = cur->wait_next;
  cur->wait_next = t;
}

void sem_take(sem_t *s) {
  for (;;) {
    uint32_t p = sched_critical_enter();
    if (s->count > 0) {
      s->count--;
      sched_critical_exit(p);
      uart_puts("[TAKE] got it immediately\r\n");
      return;
    }
    g_current_task->state = TASK_BLOCKED;
    wq_insert(s, g_current_task);   /* must stay inside critical section */
    sched_block_locked();           /* state+queue+next-seçim+PendSV interrupt must be close !!! */
    sched_critical_exit(p);
    uart_puts("[TAKE] blocking now\r\n");
    __asm volatile("dsb");
    __asm volatile("isb");
    /* woke up — retry */
  }
}

void sem_give(sem_t *s) {
  uint32_t p = sched_critical_enter();
  task_t *waiter = wq_pop(s);
  if (waiter != NULL) {
    if (s->count < s->max)
      s->count++;          /* hand off the token so sem_take's re-check succeeds */
    uart_puts("[GIVE] found waiter, waking\r\n");
    sched_wake_task(waiter); // This was bug !!
    sched_critical_exit(p);
    return;
  }
  uart_puts("[GIVE] no waiter, count++\r\n");
  if (s->count < s->max) {
    s->count++;
  }
  sched_critical_exit(p);
}

int sem_trytake(sem_t *s) {
  uint32_t p = sched_critical_enter();

  if (s->count > 0) {
    s->count--;
    sched_critical_exit(p);
    return 1;
  }

  sched_critical_exit(p);
  return 0;
}