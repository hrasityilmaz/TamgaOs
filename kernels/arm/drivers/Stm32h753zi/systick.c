#include "systick.h"
#include "scheduler.h"
#include <stdint.h>

#define SYST_CSR (*(volatile uint32_t *)0xE000E010U)
#define SYST_RVR (*(volatile uint32_t *)0xE000E014U)
#define SYST_CVR (*(volatile uint32_t *)0xE000E018U)

#define SYST_CSR_ENABLE   (1UL << 0U)
#define SYST_CSR_TICKINT  (1UL << 1U)
/* CLKSOURCE=0: AHB/8 — STM32H7 SysTick uses AHB/8 */

static volatile uint32_t s_ticks = 0U;

void systick_init(uint32_t core_clock_hz) {
    SYST_CSR = 0U;
    SYST_RVR = (core_clock_hz / 8U / 1000U) - 1U;   /* AHB/8 → 1ms */
    SYST_CVR = 0U;
    SYST_CSR = SYST_CSR_TICKINT | SYST_CSR_ENABLE;   /* CLKSOURCE=0 */
}

void SysTick_Handler(void) {
    s_ticks++;
    if (sched_is_started()) {
        sched_tick();
    }
}

void systick_delay_ms(uint32_t ms) {
    uint32_t start = s_ticks;
    while ((s_ticks - start) < ms) {}
}

uint32_t systick_get_ms(void) { return s_ticks; }