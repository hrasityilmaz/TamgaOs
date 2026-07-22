#include "sched_critical.h"
#include "scheduler.h"
#include "tamgaos_mmio.h"
#include "task.h"
#include <stddef.h>
#include "uart.h"

task_t *volatile g_current_task;
task_t *volatile g_next_task;
static volatile uint8_t s_started;

uint8_t sched_is_started(void) {
  return s_started;
}

static uint8_t s_task_count;
static task_t s_tasks[TASK_MAX];
static task_t s_idle_tcb;

/*
 * Stack canary: placed one word ABOVE the MPU-guarded region
 * (stack[0..7], 32 bytes = 8 words, see STACK_GUARD_SIZE_WORDS below),
 * so that periodic canary reads (sched_check_stack_canaries) never
 * themselves trigger the MPU no-access fault.
 */
#define TASK_STACK_CANARY_INDEX (8U)
#define TASK_STACK_CANARY_VALUE (0xDEADBEEFU)
#define TASK_STACK_CANARY_ALL_OK (0xFFU)

/* --- Core System Control Block registers (ARMv7-M architectural) --- */
#define SCB_ICSR        TAMGAOS_REG32(0xE000ED04U)
#define SCB_SHP_PENDSV  TAMGAOS_REG8(0xE000ED22U)
#define SCB_SHP_SYSTICK TAMGAOS_REG8(0xE000ED23U)
#define PENDSVSET       (1UL << 28U)

/* --- MPU registers - stack-overflow guard region --- */
#define MPU_CTRL (TAMGAOS_REG32(0xE000ED94U))
#define MPU_RNR  (TAMGAOS_REG32(0xE000ED98U))
#define MPU_RBAR (TAMGAOS_REG32(0xE000ED9CU))
#define MPU_RASR (TAMGAOS_REG32(0xE000EDA0U))

#define MPU_CTRL_ENABLE     (1UL << 0U)
#define MPU_CTRL_PRIVDEFENA (1UL << 2U)

#define STACK_GUARD_REGION      (7U)
#define STACK_GUARD_SIZE_WORDS  (8U)  /* 8 words = 32 bytes, MPU region minimum */

/* MPU_RASR field encoding (ARMv7-M Architecture Reference Manual, MPU chapter) */
#define MPU_RASR_XN_SHIFT       (28U) /* Execute-Never                       */
#define MPU_RASR_AP_SHIFT       (24U) /* Access Permission                   */
#define MPU_RASR_AP_NOACCESS    (0UL << MPU_RASR_AP_SHIFT)
#define MPU_RASR_SIZE_SHIFT     (1U)  /* Region size field                   */
#define MPU_RASR_SIZE_32B       (4UL << MPU_RASR_SIZE_SHIFT) /* 2^(4+1)=32B  */
#define MPU_RASR_ENABLE         (1UL << 0U)

static void mpu_init_stack_guard(void) {
  MPU_CTRL = 0U;
  MPU_CTRL = MPU_CTRL_ENABLE | MPU_CTRL_PRIVDEFENA;
  __asm volatile("dsb");
  __asm volatile("isb");
}

/*
 * TODO:
 * must checkk later sched_tick is not perfect for this
 * need change 
 */
void sched_wait_list_remove(task_t **head, task_t *t) {
  if (*head == t) {
    *head = t->wait_next;
    return;
  }
  task_t *cur = *head;
  while ((cur != NULL) && (cur->wait_next != t)) {
    cur = cur->wait_next;
  }
  if (cur != NULL) {
    cur->wait_next = t->wait_next;
  }
}

/*
 * Marks the lowest STACK_GUARD_SIZE_WORDS words of the given task's
 * stack as no-access (read/write/execute), so that a genuine stack
 * overflow (the task's SP crossing into this region) triggers an
 * immediate, deterministic MemManage/HardFault instead of silently
 * corrupting an adjacent task's control block.
 *
 * MISRA-C:2012 Rule 11.4 - Justified Deviation: converting the stack
 * base address to uint32_t is required by the MPU_RBAR register
 * interface, which is architecturally defined as a 32-bit value.
 */
static void mpu_set_stack_guard(const task_t *t) {
  uint32_t base = (uint32_t)(uintptr_t)&t->stack[0]; /* must be 32-byte aligned */

  MPU_RNR = STACK_GUARD_REGION;
  MPU_RBAR = base;
  MPU_RASR = (1UL << MPU_RASR_XN_SHIFT)
           | MPU_RASR_AP_NOACCESS
           | MPU_RASR_SIZE_32B
           | MPU_RASR_ENABLE;
  __asm volatile("dsb");
  __asm volatile("isb");
}

static void idle_task_func(void) {
  for (;;) {
    __asm volatile("wfi");
  }
}

static inline void trigger_pendsv(void) {
  SCB_ICSR = PENDSVSET;
  __asm volatile("dsb");
  __asm volatile("isb");
}

/*
 * Builds the initial stack frame for a not-yet-run task, matching the
 * layout expected by SVC_Handler / PendSV_Handler (kernel/arch/<cpu>/):
 *   R4-R11, EXC_RETURN, [hardware exception frame: R0-R3,R12,LR,PC,xPSR]
 *
 * MISRA-C:2012 Rule 11.1 - Justified Deviation: converting the task
 * entry-point function pointer to an integer is required to place it
 * into the simulated PC slot of the exception frame; there is no
 * standard-C alternative for constructing an initial CPU context.
 * The conversion is routed through uintptr_t (the standard-mandated
 * integer type for holding a pointer value) rather than a direct cast
 * to uint32_t, to make the intent explicit and portable.
 */
static uint32_t *task_stack_init(uint32_t *sp, void (*func)(void)) {
  sp = (uint32_t *)((uintptr_t)sp & ~(uintptr_t)0x7U);

  /* Hardware stack frame (unstacked automatically on exception return) */
  *(--sp) = 0x01000000U;                          /* xPSR (Thumb bit) */
  *(--sp) = ((uint32_t)(uintptr_t)func) | 1U;      /* PC (bit0=1: Thumb) */
  *(--sp) = 0xFFFFFFFDU;                           /* LR (no FPU)       */
  *(--sp) = 0U;                                    /* R12 */
  *(--sp) = 0U;                                    /* R3  */
  *(--sp) = 0U;                                    /* R2  */
  *(--sp) = 0U;                                    /* R1  */
  *(--sp) = 0U;                                    /* R0  */

  /* Software frame (saved/restored by PendSV_Handler) */
  *(--sp) = 0xFFFFFFFDU;                           /* EXC_RETURN (no FPU) */
  *(--sp) = 0U;                                    /* R11 */
  *(--sp) = 0U;                                    /* R10 */
  *(--sp) = 0U;                                    /* R9  */
  *(--sp) = 0U;                                    /* R8  */
  *(--sp) = 0U;                                    /* R7  */
  *(--sp) = 0U;                                    /* R6  */
  *(--sp) = 0U;                                    /* R5  */
  *(--sp) = 0U;                                    /* R4  */

  return sp;
}

void sched_init(void) {
  mpu_init_stack_guard();
  g_current_task = NULL;
  g_next_task = NULL;
  s_task_count = 0U;
  SCB_SHP_PENDSV = 0xFFU;
  SCB_SHP_SYSTICK = 0xFFU;
  s_idle_tcb.priority = TASK_PRIORITY_IDLE;
  s_idle_tcb.state = TASK_READY;
  s_idle_tcb.func = idle_task_func;
  s_idle_tcb.sp = task_stack_init(&s_idle_tcb.stack[TASK_STACK_SIZE], idle_task_func);
  s_idle_tcb.stack[TASK_STACK_CANARY_INDEX] = TASK_STACK_CANARY_VALUE;
}

int8_t sched_task_create(void (*func)(void), uint8_t priority) {
  if ((s_task_count >= TASK_MAX) || (func == NULL)) {
    return -1;
  }

  uint32_t p = sched_critical_enter();
  task_t *t = &s_tasks[s_task_count];
  t->func = func;
  t->priority = priority;
  t->state = TASK_READY;
  t->delay_ticks = 0U;
  t->wait_list_head = NULL;
  t->timed_out = 0U;
  t->sp = task_stack_init(&t->stack[TASK_STACK_SIZE], func);
  t->stack[TASK_STACK_CANARY_INDEX] = TASK_STACK_CANARY_VALUE;
  s_task_count++;
  sched_critical_exit(p);

  return (int8_t)(s_task_count - 1U);
}

static task_t *sched_select_next(void) {
  uint8_t best = 0xFFU;
  task_t *chosen = &s_idle_tcb;

  for (uint8_t i = 0U; i < s_task_count; i++) {
    if ((s_tasks[i].state == TASK_READY) && (s_tasks[i].priority < best)) {
      best = s_tasks[i].priority;
      chosen = &s_tasks[i];
    }
  }

  if ((chosen != &s_idle_tcb) && (g_current_task != NULL)) {
    uint8_t start = 0U;
    for (uint8_t i = 0U; i < s_task_count; i++) {
      if (&s_tasks[i] == g_current_task) {
        start = i;
        break;
      }
    }
    for (uint8_t k = 1U; k < s_task_count; k++) {
      uint8_t idx = (uint8_t)((start + k) % s_task_count);
      if ((s_tasks[idx].state == TASK_READY) && (s_tasks[idx].priority == best)) {
        chosen = &s_tasks[idx];
        break;
      }
    }
  }

  return chosen;
}

/*
 * Central, single point of truth for "who runs next".
 */
static void sched_commit_next(void) {
  if ((g_next_task != NULL) && (g_next_task != g_current_task) &&
      (g_next_task->state == TASK_RUNNING)) {
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
    if ((t->state == TASK_BLOCKED) && (t->delay_ticks > 0U)) {
      t->delay_ticks--;
      if (t->delay_ticks == 0U) {
        if (t->wait_list_head != NULL) {
          sched_wait_list_remove(t->wait_list_head, t);
          t->wait_list_head = NULL;
          t->timed_out = 1U;
        }
        t->state = TASK_READY;
      }
    }
  }

  if ((g_current_task != NULL) && (g_current_task->state == TASK_RUNNING)) {
    g_current_task->state = TASK_READY;
  }

  sched_commit_next();

  if (g_next_task != g_current_task) {
    trigger_pendsv();
  }
}

void sched_delay_ms(uint32_t ms) {
  if (ms == 0U) {
    return;
  }

  uint32_t p = sched_critical_enter();
  g_current_task->delay_ticks = ms;
  g_current_task->state = TASK_BLOCKED;
  sched_commit_next();
  sched_critical_exit(p);

  trigger_pendsv();
  __asm volatile("dsb");
  __asm volatile("isb");

  /* Must be volatile: read repeatedly from a location mutated by
   * sched_tick() running in interrupt context. */
  volatile uint32_t *ticks = &(g_current_task->delay_ticks);
  while (*ticks > 0U) {
    __asm volatile("wfi");
  }
}

void sched_task_pause(uint8_t index) {
  if (index >= s_task_count) {
    return;
  }
  uint32_t p = sched_critical_enter();
  s_tasks[index].state = TASK_PAUSED;
  sched_critical_exit(p);
}

void sched_task_resume(uint8_t index) {
  if (index >= s_task_count) {
    return;
  }
  uint32_t p = sched_critical_enter();
  if (s_tasks[index].state == TASK_PAUSED) {
    s_tasks[index].state = TASK_READY;
  }
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
  if ((g_current_task != NULL) && (g_current_task->state == TASK_RUNNING)) {
    g_current_task->state = TASK_READY;
  }
  sched_commit_next();
  return g_next_task;
}

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
  for (;;) {
    /* unreachable: sched_start_asm() never returns */
  }
}

void sched_wake_task(task_t *t) {
  if (t == NULL) {
    return;
  }

  uint32_t p = sched_critical_enter();
  t->wait_next = NULL;
  t->delay_ticks = 0U;
  t->state = TASK_READY;

  if ((g_current_task != NULL) && (g_current_task->state == TASK_RUNNING)) {
    g_current_task->state = TASK_READY;
  }

  sched_commit_next();
  sched_critical_exit(p);

  if (g_next_task != g_current_task) {
    trigger_pendsv();
  }
}

uint8_t sched_check_stack_canaries(void) {
  uint8_t corrupted = TASK_STACK_CANARY_ALL_OK;
  for (uint8_t i = 0U; i < s_task_count; i++) {
    if (s_tasks[i].stack[TASK_STACK_CANARY_INDEX] != TASK_STACK_CANARY_VALUE) {
      corrupted = i;
      break;
    }
  }
  return corrupted;
}