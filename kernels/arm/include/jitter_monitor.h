#ifndef JITTER_MONITOR_H
#define JITTER_MONITOR_H

#include <stdint.h>

/*
 * jitter_monitor.h measures how consistently a periodic task wakes
 * up relative to its expected schedule, independent of how long the
 * task itself takes to run (that's deadline_monitor's job).
 *
 *   jitter_handle_t h = jitter_monitor_create("flight_ctrl", 5U);
 *   for (;;) {
 *       jitter_monitor_mark(h);
 *       ... periodic ...
 *       sched_delay_ms(5U);
 *   }
 *
 */

#define JITTER_MONITOR_MAX_COUNT  8U

typedef int8_t jitter_handle_t;

jitter_handle_t jitter_monitor_create(const char *name, uint32_t period_ms);
void jitter_monitor_mark(jitter_handle_t handle);
void jitter_monitor_report(jitter_handle_t handle);
void jitter_monitor_report_all(void);

#endif /* JITTER_MONITOR_H */