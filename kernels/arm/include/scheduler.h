#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"
#include <stdint.h>

// Defines on assembly
extern task_t *volatile g_current_task;
extern task_t *volatile g_next_task;

void sched_init(void);
uint32_t sched_critical_enter(void);
void sched_critical_exit(uint32_t old_basepri);
int8_t sched_task_create(void (*func)(void), uint8_t priority);
void sched_delay_ms(uint32_t ms);
void sched_task_pause(uint8_t index);
void sched_task_resume(uint8_t index);
void sched_start(void);
void sched_tick(void);

#endif
