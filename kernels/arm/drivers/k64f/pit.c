#include "MK64F12REGS.h"
#include "mmio_deviation.h"
#include "pit.h"
#include "scheduler.h"
#include "system_clock.h"
#include <stddef.h>
#include <stdint.h>

static volatile uint32_t g_pit_tick;
static pit_callback_t s_callback = NULL;
static volatile uint8_t s_sched_active = 0U;

void pit_set_callback(pit_callback_t cb) { s_callback = cb; }

void pit_init(uint32_t period_us) {
  SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;
  
   __asm volatile("nop"); // wait a little debug->release problem was this!!!! 
  
  PIT->MCR = PIT_MCR_FRZ_MASK;
  PIT->CHANNEL[0].LDVAL = ((PIT_CLOCK_HZ / 1000000UL) * period_us) - 1UL;
  PIT->CHANNEL[0].TFLG = PIT_TFLG_TIF_MASK;
  PIT->CHANNEL[0].TCTRL = PIT_TCTRL_TEN_MASK | PIT_TCTRL_TIE_MASK;

  volatile uint8_t *nvic_ipr = (volatile uint8_t *)0xE000E400UL;
  nvic_ipr[48] = 0x40U;
  
  // TODO: check here can be problem!!!!!
  // changed from basepri critic 
  volatile uint32_t *nvic_iser = (volatile uint32_t *)0xE000E100UL;
  nvic_iser[PIT0_IRQ_NUMBER / 32U] = (1UL << (PIT0_IRQ_NUMBER % 32U));
}

uint32_t pit_get_tick(void) { return g_pit_tick; }

void pit_delay_ms(uint32_t ms) {
  uint32_t start = pit_get_tick();
  while ((pit_get_tick() - start) < ms) {
  }
}

void pit_sched_enable(void) { s_sched_active = 1U; }

void PIT0_IRQHandler(void) {
  PIT->CHANNEL[0].TFLG = PIT_TFLG_TIF_MASK;
  g_pit_tick++;
  // sched_tick();
  if (s_sched_active)
    sched_tick();
  if (s_callback != NULL) {
    s_callback();
  }
}
