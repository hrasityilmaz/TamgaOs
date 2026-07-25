/*
 * timer.c — board-agnostic software timer service.
 * no dynamic allocation
 * LOGIC MUST CHECK !!!
 */

#include "timer.h"
#include "systick.h"
#include "sched_critical.h"

typedef struct {
    bool               in_use;
    bool               active;
    bool               auto_reload;
    uint32_t           period_ms;
    uint32_t           deadline_ms;
    timer_callback_t   callback;
    void              *arg;
} timer_t;

static timer_t s_timers[TIMER_MAX_COUNT];

timer_handle_t timer_create(uint32_t period_ms, bool auto_reload, timer_callback_t callback, void *arg)
{
    if (callback == 0) {
        return -1;
    }

    uint32_t saved = sched_critical_enter();
    timer_handle_t handle = -1;
    for (uint8_t i = 0U; i < TIMER_MAX_COUNT; i++) {
        if (!s_timers[i].in_use) {
            s_timers[i].in_use      = true;
            s_timers[i].active      = false;
            s_timers[i].auto_reload = auto_reload;
            s_timers[i].period_ms   = period_ms;
            s_timers[i].deadline_ms = 0U;
            s_timers[i].callback    = callback;
            s_timers[i].arg         = arg;
            handle = (timer_handle_t)i;
            break;
        }
    }

    sched_critical_exit(saved);
    return handle;
}

void timer_start(timer_handle_t handle)
{
    if (handle < 0 || handle >= (timer_handle_t)TIMER_MAX_COUNT) {
        return;
    }

    uint32_t saved = sched_critical_enter();
    if (s_timers[handle].in_use) {
        s_timers[handle].deadline_ms = systick_get_ms() + s_timers[handle].period_ms;
        s_timers[handle].active = true;
    }
    sched_critical_exit(saved);
}

void timer_stop(timer_handle_t handle)
{
    if (handle < 0 || handle >= (timer_handle_t)TIMER_MAX_COUNT) {
        return;
    }

    uint32_t saved = sched_critical_enter();
    if (s_timers[handle].in_use) {
        s_timers[handle].active = false;
    }
    sched_critical_exit(saved);
}

void timer_set_period(timer_handle_t handle, uint32_t period_ms)
{
    if (handle < 0 || handle >= (timer_handle_t)TIMER_MAX_COUNT) {
        return;
    }

    uint32_t saved = sched_critical_enter();
    if (s_timers[handle].in_use) {
        s_timers[handle].period_ms = period_ms;
    }
    sched_critical_exit(saved);
}

void timer_delete(timer_handle_t handle)
{
    if (handle < 0 || handle >= (timer_handle_t)TIMER_MAX_COUNT) {
        return;
    }

    uint32_t saved = sched_critical_enter();
    s_timers[handle].in_use = false;
    s_timers[handle].active = false;
    sched_critical_exit(saved);
}

void timer_service_tick(void)
{
    uint32_t now = systick_get_ms();

    for (uint8_t i = 0U; i < TIMER_MAX_COUNT; i++) {
        if (!s_timers[i].in_use || !s_timers[i].active) {
            continue;
        }

        if ((int32_t)(now - s_timers[i].deadline_ms) >= 0) {
            timer_callback_t cb = s_timers[i].callback;
            void *arg = s_timers[i].arg;

            if (s_timers[i].auto_reload) {
                s_timers[i].deadline_ms += s_timers[i].period_ms;
            } else {
                s_timers[i].active = false;
            }

            if (cb != 0) {
                cb(arg);
            }
        }
    }
}

uint32_t timer_get_next_ready_in_ms(void)
{
    uint32_t now = systick_get_ms();
    uint32_t soonest = 0xFFFFFFFFUL;

    uint32_t saved = sched_critical_enter();
    for (uint8_t i = 0U; i < TIMER_MAX_COUNT; i++) {
        if (s_timers[i].in_use && s_timers[i].active) {
            int32_t remaining = (int32_t)(s_timers[i].deadline_ms - now);
            uint32_t remaining_ms = (remaining > 0) ? (uint32_t)remaining : 0U;
            if (remaining_ms < soonest) soonest = remaining_ms;
        }
    }
    sched_critical_exit(saved);
    return soonest;
}