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
#include "timer.h"



#define SYST_CSR (*(volatile uint32_t *)0xE000E010U)
#define SYST_RVR (*(volatile uint32_t *)0xE000E014U)
#define SYST_CVR (*(volatile uint32_t *)0xE000E018U)

#define SYST_CSR_ENABLE    (1UL << 0U)
#define SYST_CSR_TICKINT   (1UL << 1U)
#define SYST_CSR_CLKSOURCE (1UL << 2U)   /* 1 = core clock */

#define SYST_CSR_ENABLE     (1UL << 0U)
#define SYST_CSR_TICKINT    (1UL << 1U)
#define SYST_CSR_CLKSOURCE  (1UL << 2U)   /* 1 = core clock */
#define SYST_CSR_COUNTFLAG  (1UL << 16U)  /* ARMv7-M SysTick standard bit — set by hardware when the counter reaches 0 */

#define TICKLESS_TICKS_PER_MS (CORE_CLOCK_HZ / 1000UL)
#define TICKLESS_MAX_RELOAD  (0x00FFFFFFUL)

static volatile uint32_t s_ticks = 0U;
static volatile uint8_t s_tickless_active = 0U;

void systick_init(uint32_t core_clock_hz)
{
    SYST_CSR = 0U;
    SYST_RVR = (core_clock_hz / 1000U) - 1U;   /* core clock -> 1ms period */
    SYST_CVR = 0U;
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_TICKINT | SYST_CSR_ENABLE;
}

void systick_advance_ms(uint32_t ms)
{
    s_ticks += ms;
}

uint32_t systick_tickless_sleep(uint32_t max_ms)
{
    if (max_ms == 0U) {
        return 0U;
    }

    uint32_t reload_ticks = max_ms * TICKLESS_TICKS_PER_MS;
    if ((reload_ticks == 0U) || (reload_ticks > TICKLESS_MAX_RELOAD)) {
        reload_ticks = TICKLESS_MAX_RELOAD;
    }

    SYST_CSR &= ~SYST_CSR_ENABLE;
    SYST_RVR = reload_ticks - 1UL;
    SYST_CVR = 0UL;
    s_tickless_active = 1U;
    SYST_CSR |= (SYST_CSR_ENABLE | SYST_CSR_TICKINT);

    __asm volatile("dsb");
    __asm volatile("wfi");
    __asm volatile("isb");

    s_tickless_active = 0U;

    uint32_t elapsed_ticks;
    if ((SYST_CSR & SYST_CSR_COUNTFLAG) != 0U) {
        elapsed_ticks = reload_ticks;
    } else {
        elapsed_ticks = (reload_ticks - 1UL) - SYST_CVR;
    }

    uint32_t elapsed_ms = elapsed_ticks / TICKLESS_TICKS_PER_MS;
    if (elapsed_ms == 0U) {
        elapsed_ms = 1U;
    }

    SYST_CSR &= ~SYST_CSR_ENABLE;
    SYST_RVR = TICKLESS_TICKS_PER_MS - 1UL;
    SYST_CVR = 0UL;
    SYST_CSR |= (SYST_CSR_ENABLE | SYST_CSR_TICKINT);

    s_ticks += elapsed_ms;
    return elapsed_ms;
}

void SysTick_Handler(void)
{
    if (s_tickless_active) {
        return; 
    }
    s_ticks++;
    if (sched_is_started()) {
        sched_tick();
    }
    timer_service_tick(); // Software timer tick
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