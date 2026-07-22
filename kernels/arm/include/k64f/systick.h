#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

void systick_init(uint32_t core_clock_hz);
uint32_t systick_get_ms(void);
void systick_delay_ms(uint32_t ms);
void systick_sched_enable(void);

#endif /* SYSTICK_H */