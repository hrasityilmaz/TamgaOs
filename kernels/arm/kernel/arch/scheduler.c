#include "pit.h"
#include "sched_critical.h"
#include "scheduler.h"
#include "task.h"
#include <stdint.h>

#define SCB_ICSR (*((volatile uint32_t *)0xE000ED04UL))
#define SCB_SHPR3 (*((volatile uint32_t *)0xE000ED20UL))
#define SCB_SHPR2 (*(volatile uint32_t *)0xE000ED1CUL)
#define PENDSV_BIT (1UL << 28U)

// PENDSV AND SVCALL MUST BE SAME LOWEST LEVEL
#define PENDSV_PRIORITY_LOWEST() (SCB_SHPR3 |= (0xFFUL << 16U))
#define SVCALL_PRIORITY_LOWEST() (SCB_SHPR2 |= (0xFFUL << 24U))
#define PENDSV_TRIGGER() (SCB_ICSR = PENDSV_BIT)

static task_t s_tasks[TASK_MAX];
static uint8_t s_task_count = 0U;

task_t *volatile g_current_task = (void *)0;
task_t *volatile g_next_task = (void *)0;

static void sched_idle_task(void) {
  for (;;) {
    __asm volatile("wfi");
  }
}

static uint32_t *stack_init(uint32_t *stack_top, void (*func)(void)) {
  //  PWSR Register
  //  31 30  29  28  27 26  25  24
  //  N  Z   C    V  -  -   -   T
  //
  // [Math]::Log(0x01000000, 2) = 24 [BIT 24]
  *(--stack_top) = 0x01000000UL;
  // R15 PC hold the function address
  *(--stack_top) = (uint32_t)func;
  // R14 LR register
  // hold the return address
  *(--stack_top) = 0xFFFFFFFDUL; // EXC_RETURN  NO FPU for now!!
  *(--stack_top) = 0x00000000UL;
  *(--stack_top) = 0x00000000UL;
  *(--stack_top) = 0x00000000UL;
  *(--stack_top) = 0x00000000UL;
  *(--stack_top) = 0x00000000UL;
  // (PendSV_Handler)
  // STMDB =>r11 -> r4 //  LDMIA r4->r11
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
  uint8_t i;

  s_task_count = 0U;
  for (i = 0U; i < TASK_MAX; i++) {
    s_tasks[i].state = TASK_DEAD;
    s_tasks[i].sp = (void *)0;
    s_tasks[i].func = (void (*)(void))0;
    s_tasks[i].priority = TASK_PRIORITY_IDLE;
    s_tasks[i].delay_ticks = 0U;
  }

  PENDSV_PRIORITY_LOWEST();
  SVCALL_PRIORITY_LOWEST();
  (void)sched_task_create(sched_idle_task, TASK_PRIORITY_IDLE);
}
