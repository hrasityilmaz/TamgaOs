/*
 * deadline_monitor.c 
 */

#include "deadline_monitor.h"
#include "systick.h"
#include "uart.h"

static inline uint32_t monitor_enter_critical(void)
{
    uint32_t primask;
    __asm volatile ("mrs %0, primask" : "=r" (primask));
    __asm volatile ("cpsid i" ::: "memory");
    return primask;
}

static inline void monitor_exit_critical(uint32_t primask)
{
    __asm volatile ("msr primask, %0" :: "r" (primask) : "memory");
}

typedef struct {
    uint8_t     in_use;
    uint8_t     active; 
    const char *name;
    uint32_t    budget_ms;
    uint32_t    start_ms;
    uint32_t    min_ms;
    uint32_t    max_ms;
    uint32_t    sum_ms;
    uint32_t    cycle_count;
    uint32_t    overrun_count;
} monitor_t;

static monitor_t s_monitors[DEADLINE_MONITOR_MAX_COUNT];

monitor_handle_t deadline_monitor_create(const char *name, uint32_t budget_ms)
{
    monitor_handle_t result = -1;

    uint32_t primask = monitor_enter_critical();
    for (uint8_t i = 0U; i < DEADLINE_MONITOR_MAX_COUNT; i++) {
        if (!s_monitors[i].in_use) {
            s_monitors[i].in_use = 1U;
            result = (monitor_handle_t)i;
            break;
        }
    }
    monitor_exit_critical(primask);

    if (result < 0) {
        return -1;
    }

    monitor_t *m = &s_monitors[result];
    m->active        = 0U;
    m->name           = name;
    m->budget_ms      = budget_ms;
    m->start_ms       = 0U;
    m->min_ms         = 0xFFFFFFFFU;
    m->max_ms         = 0U;
    m->sum_ms         = 0U;
    m->cycle_count    = 0U;
    m->overrun_count  = 0U;

    return result;
}

void deadline_monitor_begin(monitor_handle_t handle)
{
    if (handle < 0 || handle >= (monitor_handle_t)DEADLINE_MONITOR_MAX_COUNT) {
        return;
    }

    monitor_t *m = &s_monitors[handle];

    if (!m->in_use) {
        return;
    }

    if (m->active) {
        uart_printf("[MONITOR] %s: begin() called while already active "
                    "(missing end()?)\r\n", m->name);
        return;
    }

    m->active    = 1U;
    m->start_ms  = systick_get_ms();
}

uint8_t deadline_monitor_end(monitor_handle_t handle)
{
    if (handle < 0 || handle >= (monitor_handle_t)DEADLINE_MONITOR_MAX_COUNT) {
        return 0U;
    }

    monitor_t *m = &s_monitors[handle];

    if (!m->in_use) {
        return 0U;
    }

    if (!m->active) {
        uart_printf("[MONITOR] %s: end() called without begin()\r\n", m->name);
        return 0U;
    }

    uint32_t elapsed = systick_get_ms() - m->start_ms;
    m->active = 0U;

    if (elapsed < m->min_ms) m->min_ms = elapsed;
    if (elapsed > m->max_ms) m->max_ms = elapsed;
    m->sum_ms += elapsed;
    m->cycle_count++;

    uint8_t overran = 0U;
    if (elapsed > m->budget_ms) {
        m->overrun_count++;
        overran = 1U;
    }

    return overran;
}

void deadline_monitor_report(monitor_handle_t handle)
{
    if (handle < 0 || handle >= (monitor_handle_t)DEADLINE_MONITOR_MAX_COUNT) {
        return;
    }

    monitor_t *m = &s_monitors[handle];
    if (!m->in_use || m->cycle_count == 0U) {
        return;
    }

    uint32_t avg_ms = m->sum_ms / m->cycle_count;

    uart_printf("[RESPONSE_TIME] %s: min=%ums avg=%ums max=%ums budget=%ums overruns=%u/%u\r\n",
                m->name, (unsigned int)m->min_ms, (unsigned int)avg_ms,
                (unsigned int)m->max_ms, (unsigned int)m->budget_ms,
                (unsigned int)m->overrun_count, (unsigned int)m->cycle_count);
}

void deadline_monitor_report_all(void)
{
    for (uint8_t i = 0U; i < DEADLINE_MONITOR_MAX_COUNT; i++) {
        if (s_monitors[i].in_use) {
            deadline_monitor_report((monitor_handle_t)i);
        }
    }
}