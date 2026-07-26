/*
 * systick.c (STM32H753ZI) — native ARM SysTick implementation.
 * Identical technique to the K64F port's systick.c: Cortex-M4 and
 * Cortex-M7 share the same SysTick register block at 0xE000E010.
 *
 */

#include "systick.h"
#include "scheduler.h"
#include <stdint.h>
#include "timer.h"

#define SYST_CSR (*(volatile uint32_t *)0xE000E010U)
#define SYST_RVR (*(volatile uint32_t *)0xE000E014U)
#define SYST_CVR (*(volatile uint32_t *)0xE000E018U)

#define SYST_CSR_ENABLE     (1UL << 0U)
#define SYST_CSR_TICKINT    (1UL << 1U)
#define SYST_CSR_COUNTFLAG  (1UL << 16U) 
/* CLKSOURCE=0: AHB/8 — STM32H7 SysTick uses AHB/8 */
#define TICKLESS_TICKS_PER_MS ((CORE_CLOCK_HZ / 8UL) / 1000UL)
#define TICKLESS_MAX_RELOAD  (0x00FFFFFFUL)

static volatile uint32_t s_ticks = 0U;
static volatile uint8_t s_tickless_active = 0U;

void systick_init(uint32_t core_clock_hz) {
    SYST_CSR = 0U;
    SYST_RVR = (core_clock_hz / 8U / 1000U) - 1U;   /* AHB/8 → 1ms */
    SYST_CVR = 0U;
    SYST_CSR = SYST_CSR_TICKINT | SYST_CSR_ENABLE;
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
    systick_isr_hook(); 
    return elapsed_ms;
}

void SysTick_Handler(void) {
    if (s_tickless_active) {
        return; 
    }
    s_ticks++;
    if (sched_is_started()) {
        sched_tick();
    }
    timer_service_tick(); /* software timer */
    systick_isr_hook(); /* isr safe hook */
}

void systick_delay_ms(uint32_t ms) {
    uint32_t start = s_ticks;
    while ((s_ticks - start) < ms) {}
}

uint32_t systick_get_ms(void) { return s_ticks; }