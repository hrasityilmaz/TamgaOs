/*
 * pit_systick_shim.c — exposes the standard "systick" API on top of
 * the PIT-based tick source (pit.c), for use when
 * TICK_SOURCE=pit is selected in board.mk. Not compiled by default.
 */

#include "systick.h"
#include "pit.h"

void systick_init(uint32_t core_clock_hz)
{
    (void)core_clock_hz;
    pit_init(1000U);       /* 1ms period */
}

uint32_t systick_get_ms(void)
{
    return pit_get_tick();
}

void systick_delay_ms(uint32_t ms)
{
    pit_delay_ms(ms);
}

void systick_sched_enable(void)
{
    pit_sched_enable();
}