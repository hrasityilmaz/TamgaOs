#include "MK64F12REGS.h"
#include "mmio_deviation.h"
#include "uart.h"

#define LED_RED_PIN_SHIFT (22U)
#define LED_RED_PIN_MASK (1UL << LED_RED_PIN_SHIFT) // PTB22
#define LED_DELAY_COUNT (500000UL)

volatile uint32_t g_systick_count = 0U;

void SysTick_Handler(void) { g_systick_count++; }

// static void delay(volatile uint32_t n) {
//   while (n != 0U) {
//     n--;
//   }
// }

static void systick_init(void) {
  SYSTICK->LOAD = ((CORE_CLOCK_HZ / 1000UL) - 1UL) & SYSTICK_LOAD_RELOAD_MASK;
  SYSTICK->VAL = 0U;
  SYSTICK->CTRL = SYSTICK_CTRL_CLKSOURCE_MASK | SYSTICK_CTRL_TICKINT_MASK |
                  SYSTICK_CTRL_ENABLE_MASK;
}

int main(void) {
  SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;               // Clock
  PORTB->PCR[LED_RED_PIN_SHIFT] = PORT_PCR_MUX(1U); // GPIO
  GPIOB->PDDR |= LED_RED_PIN_MASK;                  // Output
  uart0_init(115200U);
  uart0_puts("TamgaOS booted!\n");
  systick_init();

  while (1) {
    // GPIOB->PTOR = LED_RED_PIN_MASK;
    // delay(LED_DELAY_COUNT);
    //  uart0_puts("TamgaOS tick!\n");

    if (g_systick_count >= 1000U) {
      uart0_puts("TAMGAOS Tick systick = % 1000 ( 1sec )\n");
      g_systick_count = 0U;
      GPIOB->PTOR = LED_RED_PIN_MASK;
    }
  }
}
