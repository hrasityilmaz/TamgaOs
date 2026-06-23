#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"
#include "sched_critical.h"
#include <stdint.h>

// Defined in assembly (pendsv_handler.s)
extern task_t *volatile g_current_task;
extern task_t *volatile g_next_task;

void sched_init(void);
int8_t sched_task_create(void (*func)(void), uint8_t priority);
void sched_delay_ms(uint32_t ms);
void sched_task_pause(uint8_t index);
void sched_task_resume(uint8_t index);
void sched_start(void);
void sched_tick(void);

#endif