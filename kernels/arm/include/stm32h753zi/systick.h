#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

void systick_init(uint32_t core_clock_hz);
void systick_delay_ms(uint32_t ms);
uint32_t systick_get_ms(void);
void systick_tick_increment(void); 

#endif /* SYSTICK_H */
