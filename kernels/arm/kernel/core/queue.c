#include "queue.h"
#include "sched_critical.h"
#include "scheduler.h"
#include "systick.h"
#include "task.h"
#include <stddef.h>
#include <string.h>

void queue_init(queue_t *q, void *buffer, uint16_t item_size, uint16_t capacity) {
  q->buffer = (uint8_t *)buffer;
  q->item_size = item_size;
  q->capacity = capacity;
  q->count = 0U;
  q->head = 0U;
  q->tail = 0U;
  q->waiters_send = NULL;
  q->waiters_receive = NULL;
}

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

static task_t *wq_pop(task_t **head) {
  task_t *t = *head;
  if (t != NULL)
    *head = t->wait_next;
  return t;
}

static void queue_copy_in(queue_t *q, const void *item) {
  uint8_t *dst = q->buffer + ((size_t)q->tail * q->item_size);
  memcpy(dst, item, q->item_size);
  q->tail = (uint16_t)((q->tail + 1U) % q->capacity);
  q->count++;
}

static void queue_copy_out(queue_t *q, void *item) {
  uint8_t *src = q->buffer + ((size_t)q->head * q->item_size);
  memcpy(item, src, q->item_size);
  q->head = (uint16_t)((q->head + 1U) % q->capacity);
  q->count--;
}

void queue_send(queue_t *q, const void *item) {
  for (;;) {
    uint32_t p = sched_critical_enter();

    if (q->count < q->capacity) {
      queue_copy_in(q, item);

      /* Space just went from N-1 to N used — if a receiver was
       * waiting on empty, wake it and that data is available :) */
      task_t *waiter = wq_pop(&q->waiters_receive);
      sched_critical_exit(p);
      if (waiter != NULL) {
        sched_wake_task(waiter);
      }
      return;
    }

    g_current_task->state = TASK_BLOCKED;
    wq_insert(&q->waiters_send, g_current_task);
    sched_block_locked();
    sched_critical_exit(p);
    /* Make sure no any waiting and all instructions already done !!! */
    /* IMPORTANT !!! */
    __asm volatile("dsb");
    __asm volatile("isb");
  }
}

int queue_send_timeout(queue_t *q, const void *item, uint32_t timeout_ms) {
  uint32_t deadline = systick_get_ms() + timeout_ms;

  for (;;) {
    uint32_t p = sched_critical_enter();

    if (q->count < q->capacity) {
      queue_copy_in(q, item);
      task_t *waiter = wq_pop(&q->waiters_receive);
      sched_critical_exit(p);
      if (waiter != NULL) {
        sched_wake_task(waiter);
      }
      g_current_task->wait_list_head = NULL;
      return 0;
    }

    uint32_t now = systick_get_ms();
    if (now >= deadline) {
      sched_critical_exit(p);
      return -1; 
    }
    uint32_t remaining = deadline - now;

    g_current_task->state = TASK_BLOCKED;
    g_current_task->delay_ticks = remaining; 
    g_current_task->wait_list_head = &q->waiters_send;
    g_current_task->timed_out = 0U;
    wq_insert(&q->waiters_send, g_current_task);
    sched_block_locked();
    sched_critical_exit(p);
    __asm volatile("dsb");
    __asm volatile("isb");

    if (g_current_task->timed_out) {
      g_current_task->timed_out = 0U;
      g_current_task->wait_list_head = NULL;
      return -1;
    }
  }
}

int queue_receive_timeout(queue_t *q, void *item, uint32_t timeout_ms) {
  uint32_t deadline = systick_get_ms() + timeout_ms;

  for (;;) {
    uint32_t p = sched_critical_enter();

    if (q->count > 0U) {
      queue_copy_out(q, item);
      task_t *waiter = wq_pop(&q->waiters_send);
      sched_critical_exit(p);
      if (waiter != NULL) {
        sched_wake_task(waiter);
      }
      g_current_task->wait_list_head = NULL;
      return 0;
    }

    uint32_t now = systick_get_ms();
    if (now >= deadline) {
      sched_critical_exit(p);
      return -1;
    }
    uint32_t remaining = deadline - now;

    g_current_task->state = TASK_BLOCKED;
    g_current_task->delay_ticks = remaining;
    g_current_task->wait_list_head = &q->waiters_receive;
    g_current_task->timed_out = 0U;
    wq_insert(&q->waiters_receive, g_current_task);
    sched_block_locked();
    sched_critical_exit(p);
    __asm volatile("dsb");
    __asm volatile("isb");

    if (g_current_task->timed_out) {
      g_current_task->timed_out = 0U;
      g_current_task->wait_list_head = NULL;
      return -1;
    }
  }
}

void queue_receive(queue_t *q, void *item) {
  for (;;) {
    uint32_t p = sched_critical_enter();

    if (q->count > 0U) {
      queue_copy_out(q, item);

      task_t *waiter = wq_pop(&q->waiters_send);
      sched_critical_exit(p);
      if (waiter != NULL) {
        sched_wake_task(waiter);
      }
      return;
    }

    g_current_task->state = TASK_BLOCKED;
    wq_insert(&q->waiters_receive, g_current_task);
    sched_block_locked();
    sched_critical_exit(p);
    __asm volatile("dsb");
    __asm volatile("isb");
  }
}

int queue_try_send(queue_t *q, const void *item) {
  uint32_t p = sched_critical_enter();
  if (q->count >= q->capacity) {
    sched_critical_exit(p);
    return 0;
  }
  queue_copy_in(q, item);
  task_t *waiter = wq_pop(&q->waiters_receive);
  sched_critical_exit(p);
  if (waiter != NULL) {
    sched_wake_task(waiter);
  }
  return 1;
}

int queue_try_receive(queue_t *q, void *item) {
  uint32_t p = sched_critical_enter();
  if (q->count == 0U) {
    sched_critical_exit(p);
    return 0;
  }
  queue_copy_out(q, item);
  task_t *waiter = wq_pop(&q->waiters_send);
  sched_critical_exit(p);
  if (waiter != NULL) {
    sched_wake_task(waiter);
  }
  return 1;
}