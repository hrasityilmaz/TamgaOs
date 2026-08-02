/*
 * jitter_monitor.c 
 */

#include "jitter_monitor.h"
#include "sched_critical.h"
#include "systick.h"
#include "uart.h"

typedef struct {
    uint8_t     in_use;
    const char *name;
    uint32_t    period_ms;
    uint32_t    expected_ms; 
    uint8_t     baseline_set;
    uint32_t    min_jitter_ms;
    uint32_t    max_jitter_ms;
    uint32_t    sum_jitter_ms;
    uint32_t    sample_count;
} jitter_t;

static jitter_t s_monitors[JITTER_MONITOR_MAX_COUNT];

jitter_handle_t jitter_monitor_create(const char *name, uint32_t period_ms)
{
    uint32_t p = sched_critical_enter();

    jitter_handle_t handle = -1;
    for (uint8_t i = 0U; i < JITTER_MONITOR_MAX_COUNT; i++) {
        if (!s_monitors[i].in_use) {
            s_monitors[i].in_use       = 1U;
            s_monitors[i].name         = name;
            s_monitors[i].period_ms    = period_ms;
            s_monitors[i].expected_ms  = 0U;
            s_monitors[i].baseline_set = 0U;
            s_monitors[i].min_jitter_ms = 0xFFFFFFFFU;
            s_monitors[i].max_jitter_ms = 0U;
            s_monitors[i].sum_jitter_ms = 0U;
            s_monitors[i].sample_count  = 0U;
            handle = (jitter_handle_t)i;
            break;
        }
    }

    sched_critical_exit(p);
    return handle;
}

void jitter_monitor_mark(jitter_handle_t handle)
{
    if (handle < 0 || handle >= (jitter_handle_t)JITTER_MONITOR_MAX_COUNT) {
        return;
    }

    jitter_t *m = &s_monitors[handle];
    if (!m->in_use) {
        return;
    }

    uint32_t now = systick_get_ms();

    if (!m->baseline_set) {
        m->expected_ms  = now + m->period_ms;
        m->baseline_set = 1U;
        return;
    }

    int32_t diff = (int32_t)(now - m->expected_ms);
    uint32_t jitter = (diff < 0) ? (uint32_t)(-diff) : (uint32_t)diff;

    if (jitter < m->min_jitter_ms) m->min_jitter_ms = jitter;
    if (jitter > m->max_jitter_ms) m->max_jitter_ms = jitter;
    m->sum_jitter_ms += jitter;
    m->sample_count++;
    m->expected_ms += m->period_ms;
}

void jitter_monitor_report(jitter_handle_t handle)
{
    if (handle < 0 || handle >= (jitter_handle_t)JITTER_MONITOR_MAX_COUNT) {
        return;
    }

    jitter_t *m = &s_monitors[handle];
    if (!m->in_use || m->sample_count == 0U) {
        return;
    }

    uint32_t avg_jitter = m->sum_jitter_ms / m->sample_count;

    uart_printf("[JITTER] %s: period=%ums min=%ums avg=%ums max=%ums samples=%u\r\n",
                m->name, (unsigned int)m->period_ms,
                (unsigned int)m->min_jitter_ms, (unsigned int)avg_jitter,
                (unsigned int)m->max_jitter_ms, (unsigned int)m->sample_count);
}

void jitter_monitor_report_all(void)
{
    for (uint8_t i = 0U; i < JITTER_MONITOR_MAX_COUNT; i++) {
        if (s_monitors[i].in_use) {
            jitter_monitor_report((jitter_handle_t)i);
        }
    }
}