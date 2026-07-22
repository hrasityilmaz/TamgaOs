#include "event.h"
#include "sched_critical.h"
#include "scheduler.h"
#include "systick.h"
#include "task.h"
#include <stddef.h>

void event_init(event_group_t *e) {
  e->bits = 0U;
  e->waiters = NULL;
}

/* Priority-ordered insert — same pattern as mutex.c/queue.c. */
static void wq_insert(task_t **head, task_t *t) {
  t->wait_next = NULL;
  if (*head == NULL || t->priority < (*head)->priority) {
    t->wait_next = *head;
    *head = t;
    return;
  }
  task_t *cur = *head;
  while (cur->wait_next != NULL && cur->wait_next->priority <= t->priority)
    cur = cur->wait_next;
  t->wait_next = cur->wait_next;
  cur->wait_next = t;
}

static int event_condition_met(uint32_t bits, uint32_t mask, event_wait_mode_t mode) {
  if (mode == EVENT_WAIT_ALL) {
    return (bits & mask) == mask;
  }
  return (bits & mask) != 0U;
}

#define EVENT_PENDING_MAX (12U)   /* must be >= TASK_MAX from task.h */

typedef struct {
  task_t *task;
  event_group_t *group;
  uint32_t mask;
  event_wait_mode_t mode;
  int auto_clear;
  uint32_t observed_bits; 
} event_pending_t;

static event_pending_t s_pending[EVENT_PENDING_MAX];

static event_pending_t *event_pending_find(task_t *t) {
  for (uint8_t i = 0U; i < EVENT_PENDING_MAX; i++) {
    if (s_pending[i].task == t) {
      return &s_pending[i];
    }
  }
  return NULL;
}

static event_pending_t *event_pending_alloc(task_t *t) {
  for (uint8_t i = 0U; i < EVENT_PENDING_MAX; i++) {
    if (s_pending[i].task == NULL) {
      s_pending[i].task = t;
      return &s_pending[i];
    }
  }
  return NULL;
}

static void event_pending_free(event_pending_t *p) {
  p->task = NULL;
}

static void event_wake_satisfied(event_group_t *e) {
  task_t **cur = &e->waiters;
  while (*cur != NULL) {
    task_t *t = *cur;
    event_pending_t *p = event_pending_find(t);

    if ((p != NULL) && event_condition_met(e->bits, p->mask, p->mode)) {
      p->observed_bits = e->bits;
      if (p->auto_clear) {
        e->bits &= ~p->mask;
      }

      *cur = t->wait_next;   /* unlink from wait list */
      t->wait_next = NULL;
      t->wait_list_head = NULL;

      sched_wake_task(t);
    } else {
      cur = &t->wait_next;
    }
  }
}

void event_set(event_group_t *e, uint32_t bits_to_set) {
  uint32_t p = sched_critical_enter();
  e->bits |= bits_to_set;
  event_wake_satisfied(e);
  sched_critical_exit(p);
}

void event_clear(event_group_t *e, uint32_t bits_to_clear) {
  uint32_t p = sched_critical_enter();
  e->bits &= ~bits_to_clear;
  sched_critical_exit(p);
}

uint32_t event_get(event_group_t *e) {
  return e->bits;
}

uint32_t event_wait(event_group_t *e, uint32_t mask,
                     event_wait_mode_t mode, int auto_clear) {
  for (;;) {
    uint32_t p = sched_critical_enter();

    if (event_condition_met(e->bits, mask, mode)) {
      uint32_t observed = e->bits;
      if (auto_clear) {
        e->bits &= ~mask;
      }
      sched_critical_exit(p);
      return observed;
    }

    event_pending_t *pend = event_pending_alloc(g_current_task);
    if (pend == NULL) {
      sched_critical_exit(p);
      continue;
    }
    pend->group = e;
    pend->mask = mask;
    pend->mode = mode;
    pend->auto_clear = auto_clear;

    g_current_task->state = TASK_BLOCKED;
    g_current_task->wait_list_head = NULL;   /* no timeout on this path */
    wq_insert(&e->waiters, g_current_task);
    sched_block_locked();
    sched_critical_exit(p);
    __asm volatile("dsb");
    __asm volatile("isb");

    uint32_t result = pend->observed_bits;
    event_pending_free(pend);
    return result;
  }
}

uint32_t event_wait_timeout(event_group_t *e, uint32_t mask,
                             event_wait_mode_t mode, int auto_clear,
                             uint32_t timeout_ms) {
  uint32_t deadline = systick_get_ms() + timeout_ms;

  for (;;) {
    uint32_t p = sched_critical_enter();

    if (event_condition_met(e->bits, mask, mode)) {
      uint32_t observed = e->bits;
      if (auto_clear) {
        e->bits &= ~mask;
      }
      sched_critical_exit(p);
      return observed;
    }

    uint32_t now = systick_get_ms();
    if (now >= deadline) {
      sched_critical_exit(p);
      return 0xFFFFFFFFU;
    }
    uint32_t remaining = deadline - now;

    event_pending_t *pend = event_pending_alloc(g_current_task);
    if (pend == NULL) {
      sched_critical_exit(p);
      return 0xFFFFFFFFU; 
    }
    pend->group = e;
    pend->mask = mask;
    pend->mode = mode;
    pend->auto_clear = auto_clear;

    g_current_task->state = TASK_BLOCKED;
    g_current_task->delay_ticks = remaining; 
    g_current_task->wait_list_head = &e->waiters;
    g_current_task->timed_out = 0U;
    wq_insert(&e->waiters, g_current_task);
    sched_block_locked();
    sched_critical_exit(p);
    __asm volatile("dsb");
    __asm volatile("isb");

    if (g_current_task->timed_out) {
      g_current_task->timed_out = 0U;
      g_current_task->wait_list_head = NULL;
      event_pending_free(pend);
      return 0xFFFFFFFFU;
    }

    uint32_t result = pend->observed_bits;
    event_pending_free(pend);
    return result;
  }
}