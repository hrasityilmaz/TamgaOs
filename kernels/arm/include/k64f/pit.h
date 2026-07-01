#ifndef PIT_H
#define PIT_H

#include <stdint.h>

typedef void (*pit_callback_t)(void);
void pit_set_callback(pit_callback_t cb);
void pit_init(uint32_t period_us);
void pit_sched_enable(void);
uint32_t pit_get_tick(void);
void pit_delay_ms(uint32_t ms);

#endif
