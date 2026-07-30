#ifndef NOTIFY_H
#define NOTIFY_H

#include <stdint.h>
#include "task.h"

/*
 * notify.h — lightweight  without queue or event group 
 */

#define TASK_NOTIFY_WAIT_FOREVER  (0xFFFFFFFFUL)

void task_notify_give(task_t *t, uint32_t value);

static inline void task_notify_give_from_isr(task_t *t, uint32_t value)
{
    task_notify_give(t, value);
}

uint8_t task_notify_wait(uint32_t *out_value, uint32_t timeout_ms);

#endif /* NOTIFY_H */