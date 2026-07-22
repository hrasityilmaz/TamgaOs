/*
 * systick.c (K64F) — native ARM SysTick implementation. Identical
 * technique to the STM32H753ZI port's systick.c: Cortex-M4 and
 * Cortex-M7 share the same SysTick register block at 0xE000E010.
 *
 * CLKSOURCE=1 (processor/core clock) is used rather than an external
 * reference clock — this is the simplest, most predictable choice
 * across vendors (Kinetis's external SysTick reference clock options
 * vary by part and aren't worth the complexity here).
 */

#include "systick.h"
#include "scheduler.h"
#include <stdint.h>

#define SYST_CSR (*(volatile uint32_t *)0xE000E010U)
#define SYST_RVR (*(volatile uint32_t *)0xE000E014U)
#define SYST_CVR (*(volatile uint32_t *)0xE000E018U)

#define SYST_CSR_ENABLE    (1UL << 0U)
#define SYST_CSR_TICKINT   (1UL << 1U)
#define SYST_CSR_CLKSOURCE (1UL << 2U)   /* 1 = core clock */

static volatile uint32_t s_ticks = 0U;

void systick_init(uint32_t core_clock_hz)
{
    SYST_CSR = 0U;
    SYST_RVR = (core_clock_hz / 1000U) - 1U;   /* core clock -> 1ms period */
    SYST_CVR = 0U;
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_TICKINT | SYST_CSR_ENABLE;
}

void SysTick_Handler(void)
{
    s_ticks++;
    if (sched_is_started()) {
        sched_tick();
    }
}

void systick_delay_ms(uint32_t ms)
{
    uint32_t start = s_ticks;
    while ((s_ticks - start) < ms) {}
}

uint32_t systick_get_ms(void)
{
    return s_ticks;
}

void systick_sched_enable(void)
{
    /* No-op — SysTick_Handler already checks sched_is_started() on
       every tick, no explicit enable step needed. Exists only for
       API symmetry with the PIT-backed alternative (see
       pit_systick_shim.c), so main.c can call this uniformly
       regardless of which TICK_SOURCE is selected. */
}