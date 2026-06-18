#include "MK64F12REGS.h"
#include "mmio_deviation.h"
#include "pit.h"
#include "system_clock.h"
#include <stdint.h>

static volatile uint32_t g_pit_tick = 0U;

void pit_init(uint32_t period_us) {
  SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;
  PIT->MCR = PIT_MCR_FRZ_MASK;

  PIT->CHANNEL[0].LDVAL = ((PIT_CLOCK_HZ / 1000000UL) * period_us) - 1UL;
  PIT->CHANNEL[0].TFLG = PIT_TFLG_TIF_MASK;
  PIT->CHANNEL[0].TCTRL = PIT_TCTRL_TEN_MASK | PIT_TCTRL_TIE_MASK;

  volatile uint32_t *nvic_iser = (volatile uint32_t *)0xE000E100UL;
  nvic_iser[PIT0_IRQ_NUMBER / 32U] = (1UL << (PIT0_IRQ_NUMBER % 32U));
}

uint32_t pit_get_tick(void) { return g_pit_tick; }

void pit_delay_ms(uint32_t ms) {
    uint32_t start = pit_get_tick();
    uint32_t now;
    do {
        now = pit_get_tick();
    } while ((now - start) < ms);
}

void PIT0_IRQHandler(void) {
    PIT->CHANNEL[0].TFLG = PIT_TFLG_TIF_MASK;
    g_pit_tick++;
}
