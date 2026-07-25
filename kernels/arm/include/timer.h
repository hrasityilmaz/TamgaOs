#ifndef KERNEL_TIMER_H
#define KERNEL_TIMER_H

#include <stdint.h>
#include <stdbool.h>

#define TIMER_MAX_COUNT   16U
typedef void (*timer_callback_t)(void *arg);
typedef int8_t timer_handle_t;
timer_handle_t timer_create(uint32_t period_ms, bool auto_reload, timer_callback_t callback, void *arg);
void timer_start(timer_handle_t handle);
void timer_stop(timer_handle_t handle);
void timer_set_period(timer_handle_t handle, uint32_t period_ms);
void timer_delete(timer_handle_t handle);
void timer_service_tick(void);

#endif /* KERNEL_TIMER_H */