#include "sched_critical.h"
#include "scheduler.h"
#include "task.h"
#include <stddef.h>

task_t *volatile g_current_task;
task_t *volatile g_next_task;
static volatile uint8_t s_started;

uint8_t sched_is_started(void) { return s_started; }

static uint8_t s_task_count;
static task_t s_tasks[TASK_MAX];
static task_t s_idle_tcb;

#define SCB_ICSR (*(volatile uint32_t *)0xE000ED04U)
#define PENDSVSET (1UL << 28U)
#define SCB_SHP_PENDSV (*(volatile uint8_t *)0xE000ED22U)
#define SCB_SHP_SYSTICK (*(volatile uint8_t *)0xE000ED23U)

// MPU AREA TEST
// TODO MUST CONTROL TEST !!!
#define MPU_CTRL   (*(volatile uint32_t *)0xE000ED94U)
#define MPU_RNR    (*(volatile uint32_t *)0xE000ED98U)
#define MPU_RBAR   (*(volatile uint32_t *)0xE000ED9CU)
#define MPU_RASR   (*(volatile uint32_t *)0xE000EDA0U)

#define MPU_CTRL_ENABLE      (1UL << 0U)
#define MPU_CTRL_PRIVDEFENA  (1UL << 2U)  
#define STACK_GUARD_REGION   7U

static void mpu_init_stack_guard(void) {
  MPU_CTRL = 0U;
  MPU_CTRL = MPU_CTRL_ENABLE | MPU_CTRL_PRIVDEFENA;
  __asm volatile("dsb");
  __asm volatile("isb");
}

static void mpu_set_stack_guard(task_t *t) {
  uint32_t base = (uint32_t)&t->stack[0];  // Must be 32Bytes aligned !!!

  MPU_RNR = STACK_GUARD_REGION;
  MPU_RBAR = base;
  MPU_RASR = (1UL << 28)   // XN: execute never
           | (0UL << 24)   // AP=000
           | (4UL << 1)    // SIZE=4 -> 2^(4+1)=32 byte
           | (1UL << 0);
  __asm volatile("dsb");
  __asm volatile("isb");
}

// END OF MPU 


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

static uint32_t *task_stack_init(uint32_t *sp, void (*func)(void))
{
  sp = (uint32_t *)((uintptr_t)sp & ~0x7);
  // Hardware stack 
  *(--sp) = 0x01000000;          // xPSR
  *(--sp) = (uint32_t)func | 1;  // PC
  *(--sp) = 0xFFFFFFFD;          // LR (no FPU)
  *(--sp) = 0;                   // R12
  *(--sp) = 0;                   // R3
  *(--sp) = 0;                   // R2
  *(--sp) = 0;                   // R1
  *(--sp) = 0;                   // R0
  // Software frame
  *(--sp) = 0xFFFFFFFD;          // EXC_RETURN (no FPU)
  *(--sp) = 0;                   // R11
  *(--sp) = 0;                   // R10
  *(--sp) = 0;                   // R9
  *(--sp) = 0;                   // R8
  *(--sp) = 0;                   // R7
  *(--sp) = 0;                   // R6
  *(--sp) = 0;                   // R5
  *(--sp) = 0;                   // R4

  return sp;
}

void sched_init(void) {
  mpu_init_stack_guard();
  g_current_task = (task_t *)0U;
  g_next_task = (task_t *)0U;
  s_task_count = 0U;
  SCB_SHP_PENDSV = 0xFFU;
  SCB_SHP_SYSTICK = 0xFFU;
  s_idle_tcb.priority = TASK_PRIORITY_IDLE;
  s_idle_tcb.state = TASK_READY;
  s_idle_tcb.func = idle_task_func;
  s_idle_tcb.sp =
      task_stack_init(&s_idle_tcb.stack[TASK_STACK_SIZE], idle_task_func);
  s_idle_tcb.stack[8] = 0xDEADBEEFU;
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
  t->sp = task_stack_init(&t->stack[TASK_STACK_SIZE], func);
  t->stack[8] = 0xDEADBEEFU; // canary
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

// Select the new one and choose this as RUNNING
// TODO : control needed !!!!
static void sched_commit_next(void) {
  if (g_next_task != NULL && g_next_task != g_current_task &&
      g_next_task->state == TASK_RUNNING) {
    g_next_task->state = TASK_READY;
  }
  g_next_task = sched_select_next();
  if (g_next_task->state == TASK_READY) {
    g_next_task->state = TASK_RUNNING;
  }
  mpu_set_stack_guard(g_next_task);
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
  if (g_current_task != NULL && g_current_task->state == TASK_RUNNING) {
    g_current_task->state = TASK_READY;
  }
  sched_commit_next();
  if (g_next_task != g_current_task)
    trigger_pendsv();
}

void sched_delay_ms(uint32_t ms) {
  if (ms == 0U)
    return;

  uint32_t p = sched_critical_enter();
  g_current_task->delay_ticks = ms;
  g_current_task->state = TASK_BLOCKED;
  sched_commit_next();
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
    sched_commit_next();
  }
  sched_critical_exit(p);
  trigger_pendsv();
  __asm volatile("dsb");
  __asm volatile("isb");
}

static task_t *sched_pick_and_mark(void) {
  if (g_current_task != NULL && g_current_task->state == TASK_RUNNING) {
    g_current_task->state = TASK_READY;
  }
  sched_commit_next();
  return g_next_task;
}

// central otherwise forgotting !!
void sched_block_locked(void) {
  g_next_task = sched_pick_and_mark();
  trigger_pendsv();
}

void sched_block(void) {
  uint32_t p = sched_critical_enter();
  g_next_task = sched_pick_and_mark();
  sched_critical_exit(p);
  trigger_pendsv();
  __asm volatile("dsb");
  __asm volatile("isb");
}

void sched_start(void) {
  s_started = 1U;
  g_current_task = sched_select_next();
  g_next_task = g_current_task;
  g_current_task->state = TASK_RUNNING;
  mpu_set_stack_guard(g_current_task);
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
  if (g_current_task != NULL && g_current_task->state == TASK_RUNNING) {
    g_current_task->state = TASK_READY;
  }
  sched_commit_next();
  sched_critical_exit(p);
  if (g_next_task != g_current_task)
    trigger_pendsv();
}

uint8_t sched_check_stack_canaries(void) {
  uint8_t corrupted = 0xFFU; /* 0xFF = ALL OK */
  for (uint8_t i = 0U; i < s_task_count; i++) {
    if (s_tasks[i].stack[8] != 0xDEADBEEFU) {
      corrupted = i;
      break;
    }
  }
  return corrupted;
}