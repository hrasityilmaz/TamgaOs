#ifndef STACK_MONITOR_H
#define STACK_MONITOR_H

#include <stdint.h>
#include "task.h"

#define STACK_FILL_BYTE  (0xA5U)

void stack_monitor_fill(task_t *t);
uint32_t stack_monitor_get_used_bytes(const task_t *t);
uint8_t stack_monitor_get_used_percent(const task_t *t);

#endif /* STACK_MONITOR_H */