/*
 * notify.c 
 */

#include "notify.h"
#include "sched_critical.h"
#include "scheduler.h"
#include <stddef.h>

extern task_t *volatile g_current_task;

void task_notify_give(task_t *t, uint32_t value)
{
    if (t == NULL) {
        return;
    }

    uint32_t p = sched_critical_enter();
    t->notify_value   = value;
    t->notify_pending = 1U;
    uint8_t was_blocked = (t->state == TASK_BLOCKED);
    sched_critical_exit(p);

    if (was_blocked) {
        sched_wake_task(t);
    }
}

uint8_t task_notify_wait(uint32_t *out_value, uint32_t timeout_ms)
{
    uint32_t p = sched_critical_enter();

    if (g_current_task->notify_pending) {
        g_current_task->notify_pending = 0U;
        if (out_value != NULL) {
            *out_value = g_current_task->notify_value;
        }
        sched_critical_exit(p);
        return 1U;
    }

    if (timeout_ms == 0U) {
        // TOOD:
        // return immediately! but need check can be problematic !!
        sched_critical_exit(p);
        return 0U;
    }

    g_current_task->state = TASK_BLOCKED; 
    g_current_task->delay_ticks   = timeout_ms;
    g_current_task->wait_list_head = NULL;
    sched_block_locked();
    sched_critical_exit(p);
    __asm volatile("dsb");
    __asm volatile("isb");

    uint32_t p2 = sched_critical_enter();
    uint8_t got = 0U;
    if (g_current_task->notify_pending) {
        g_current_task->notify_pending = 0U;
        if (out_value != NULL) {
            *out_value = g_current_task->notify_value;
        }
        got = 1U;
    }
    sched_critical_exit(p2);

    return got;
}