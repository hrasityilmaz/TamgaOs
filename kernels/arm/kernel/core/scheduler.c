#include "sched_critical.h"
#include "scheduler.h"
#include "task.h"
#include <stddef.h>

task_t *volatile g_current_task;
task_t *volatile g_next_task;

static uint8_t s_task_count;
static task_t s_tasks[TASK_MAX];
static task_t s_idle_tcb;

#define SCB_ICSR (*(volatile uint32_t *)0xE000ED04U)
#define PENDSVSET (1UL << 28U)
#define SCB_SHP_PENDSV (*(volatile uint8_t *)0xE000ED22U)
#define SCB_SHP_SYSTICK (*(volatile uint8_t *)0xE000ED23U)

static void idle_task_func(void) {
  while (1) {
    __asm volatile("wfi");
  }
}

static inline void trigger_pendsv(void) {
  SCB_ICSR = PENDSVSET;
  __asm volatile("dsb");
  __asm volatile("isb");
}

static uint32_t *task_stack_init(uint32_t *stack_top, void (*func)(void)) {
  stack_top = (uint32_t *)((uint32_t)stack_top & ~0x7UL);
  *(--stack_top) = 0x01000000UL;
  *(--stack_top) = (uint32_t)func | 1U;
  *(--stack_top) = 0xFFFFFFFDUL;
  *(--stack_top) = 0x00000000UL;
  *(--stack_top) = 0x00000000UL;
  *(--stack_top) = 0x00000000UL;
  *(--stack_top) = 0x00000000UL;
  *(--stack_top) = 0x00000000UL;
  *(--stack_top) = 0x00000000UL;
  *(--stack_top) = 0x00000000UL;
  *(--stack_top) = 0x00000000UL;
  *(--stack_top) = 0x00000000UL;
  *(--stack_top) = 0x00000000UL;
  *(--stack_top) = 0x00000000UL;
  *(--stack_top) = 0x00000000UL;
  *(--stack_top) = 0x00000000UL;
  return stack_top;
}

void sched_init(void) {
  g_current_task = (task_t *)0U;
  g_next_task = (task_t *)0U;
  s_task_count = 0U;
  SCB_SHP_PENDSV = 0xFFU;
  SCB_SHP_SYSTICK = 0xFFU;
  s_idle_tcb.priority = TASK_PRIORITY_IDLE;
  s_idle_tcb.state = TASK_READY;
  s_idle_tcb.func = idle_task_func;
  s_idle_tcb.wait_next = NULL;
  s_idle_tcb.sp =
      task_stack_init(&s_idle_tcb.stack[TASK_STACK_SIZE], idle_task_func);
}

int8_t sched_task_create(void (*func)(void), uint8_t priority) {
  if (s_task_count >= TASK_MAX || func == NULL)
    return -1;
  uint32_t p = sched_critical_enter();
  task_t *t = &s_tasks[s_task_count];
  t->func = func;
  t->priority = priority;
  t->state = TASK_READY;
  t->delay_ticks = 0U;
  t->wait_next = NULL;
  t->sp = task_stack_init(&t->stack[TASK_STACK_SIZE], func);
  s_task_count++;
  sched_critical_exit(p);
  return (int8_t)(s_task_count - 1U);
}

static task_t *sched_select_next(void) {
  uint8_t best = 0xFFU;
  task_t *chosen = &s_idle_tcb;
  for (uint8_t i = 0U; i < s_task_count; i++) {
    if (s_tasks[i].state == TASK_READY && s_tasks[i].priority < best) {
      best = s_tasks[i].priority;
      chosen = &s_tasks[i];
    }
  }
  if (chosen != &s_idle_tcb && g_current_task != NULL) {
    uint8_t start = 0U;
    for (uint8_t i = 0U; i < s_task_count; i++) {
      if (&s_tasks[i] == g_current_task) {
        start = i;
        break;
      }
    }
    for (uint8_t k = 1U; k < s_task_count; k++) {
      uint8_t idx = (start + k) % s_task_count;
      if (s_tasks[idx].state == TASK_READY && s_tasks[idx].priority == best) {
        chosen = &s_tasks[idx];
        break;
      }
    }
  }
  return chosen;
}

/*
 * Picks the next task and fixes up RUNNING/READY bookkeeping.
 * MUST be called with interrupts already disabled (inside a critical
 * section) since it touches g_current_task->state.
 *
 * Caller is responsible for setting g_current_task's state to whatever
 * it should be BEFORE calling this (TASK_BLOCKED, TASK_READY for yield,
 * etc). If the caller doesn't touch it, it defaults back to READY here
 * (this covers sched_tick's preemption case).
 */
static task_t *sched_pick_and_mark(void) {
  if (g_current_task != NULL && g_current_task->state == TASK_RUNNING)
    g_current_task->state = TASK_READY;

  task_t *next = sched_select_next();
  next->state = TASK_RUNNING;
  return next;
}

void sched_tick(void) {
  for (uint8_t i = 0U; i < s_task_count; i++) {
    task_t *t = &s_tasks[i];
    if (t->state == TASK_BLOCKED && t->delay_ticks > 0U) {
      t->delay_ticks--;
      if (t->delay_ticks == 0U)
        t->state = TASK_READY;
    }
  }
  g_next_task = sched_pick_and_mark();
  if (g_next_task != g_current_task)
    trigger_pendsv();
}

void sched_delay_ms(uint32_t ms) {
  if (ms == 0U)
    return;

  uint32_t p = sched_critical_enter();
  g_current_task->delay_ticks = ms;
  g_current_task->state = TASK_BLOCKED;
  g_next_task = sched_pick_and_mark();
  sched_critical_exit(p);

  trigger_pendsv();
  __asm volatile("dsb");
  __asm volatile("isb");

  /* ticks MUST be volatile it was a bug !! */
  volatile uint32_t *ticks = &(g_current_task->delay_ticks);
  while (*ticks > 0U) {
    __asm volatile("wfi");
  }
}

void sched_task_pause(uint8_t index) {
  if (index >= s_task_count)
    return;
  uint32_t p = sched_critical_enter();
  s_tasks[index].state = TASK_PAUSED;
  sched_critical_exit(p);
}

void sched_task_resume(uint8_t index) {
  if (index >= s_task_count)
    return;
  uint32_t p = sched_critical_enter();
  if (s_tasks[index].state == TASK_PAUSED)
    s_tasks[index].state = TASK_READY;
  sched_critical_exit(p);
}

void sched_yield(void) {
  uint32_t p = sched_critical_enter();
  if (g_current_task != NULL) {
    g_current_task->state = TASK_READY;
    g_next_task = sched_pick_and_mark();
  }
  sched_critical_exit(p);
  trigger_pendsv();
  __asm volatile("dsb");
  __asm volatile("isb");
}

/*
 * Called by sem_take / mutex_lock AFTER they have already set
 * g_current_task->state to TASK_BLOCKED and inserted it into a wait
 * queue. Unlike sched_yield, this does NOT touch g_current_task->state —
 * it must stay BLOCKED until something explicitly wakes it via
 * sched_wake_task.
 */
void sched_block(void) {
  uint32_t p = sched_critical_enter();
  g_next_task = sched_pick_and_mark();
  sched_critical_exit(p);
  trigger_pendsv();
  __asm volatile("dsb");
  __asm volatile("isb");
}

void sched_start(void) {
  g_current_task = sched_select_next();
  g_next_task = g_current_task;
  g_current_task->state = TASK_RUNNING;
  sched_start_asm(g_current_task);
  while (1) {
  }
}

void sched_wake_task(task_t *t) {
  if (t == NULL)
    return;

  uint32_t p = sched_critical_enter();
  t->wait_next = NULL;
  t->delay_ticks = 0U;
  t->state = TASK_READY;
  g_next_task = sched_pick_and_mark();
  sched_critical_exit(p);

  if (g_next_task != g_current_task)
    trigger_pendsv();
}