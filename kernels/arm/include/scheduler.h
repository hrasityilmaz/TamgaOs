#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"
#include <stdint.h>

extern task_t *volatile g_current_task;
extern task_t *volatile g_next_task;

void sched_init(void);
int8_t sched_task_create(void (*func)(void), uint8_t priority);
void sched_delay_ms(uint32_t ms);
void sched_yield(void);
void sched_start_asm(task_t *task);
void sched_task_pause(uint8_t index);
void sched_task_resume(uint8_t index);
void sched_start(void);
void sched_tick(void);
void sched_wake_task(task_t *t);
void sched_block(void);
uint8_t sched_is_started(void);
void sched_block_locked(void);
uint8_t sched_check_stack_canaries(void);
#endif
