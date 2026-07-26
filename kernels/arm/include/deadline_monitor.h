#ifndef KERNEL_DEADLINE_MONITOR_H
#define KERNEL_DEADLINE_MONITOR_H

#include <stdint.h>


#define DEADLINE_MONITOR_MAX_COUNT  8U

typedef int8_t monitor_handle_t;

monitor_handle_t deadline_monitor_create(const char *name, uint32_t budget_ms);
void deadline_monitor_begin(monitor_handle_t handle);
uint8_t deadline_monitor_end(monitor_handle_t handle);
void deadline_monitor_report(monitor_handle_t handle);
void deadline_monitor_report_all(void);

#endif /* KERNEL_DEADLINE_MONITOR_H */