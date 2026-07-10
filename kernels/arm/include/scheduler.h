#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"
#include <stdint.h>

/*
 * g_current_task / g_next_task: mutated by PendSV_Handler / SVC_Handler
 * (assembly, kernel/arch/<cpu>/) and by the scheduler functions below.
 * "volatile" qualifies the pointer itself (its value can change
 * asynchronously via interrupt context), not the pointee.
 */
extern task_t *volatile g_current_task;
extern task_t *volatile g_next_task;

/*  Initialization  */
void sched_init(void);

/*  Task creation / lifecycle  */
int8_t sched_task_create(void (*func)(void), uint8_t priority);
void sched_task_pause(uint8_t index);
void sched_task_resume(uint8_t index);

/* Scheduling primitives (voluntary and involuntary yield points)  */
void sched_start(void);
void sched_start_asm(task_t *task);
void sched_tick(void);
void sched_delay_ms(uint32_t ms);
void sched_yield(void);
void sched_block(void);
void sched_block_locked(void);
void sched_wake_task(task_t *t);
uint8_t sched_is_started(void);

/*  Diagnostics  */
uint8_t sched_check_stack_canaries(void);

#endif /* SCHEDULER_H */