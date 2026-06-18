#ifndef PIT_H
#define PIT_H

#include <stdint.h>

void pit_init(uint32_t period_us);
uint32_t pit_get_tick(void);
void pit_delay_ms(uint32_t ms);

#endif
