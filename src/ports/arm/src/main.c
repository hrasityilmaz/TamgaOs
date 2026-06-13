#include "mmio_deviation.h"
#include "uart.h"

#define LED_RED_PIN_SHIFT (22U)
#define LED_RED_PIN_MASK (1UL << LED_RED_PIN_SHIFT) /* PTB22 */
#define LED_DELAY_COUNT (500000UL)

static void delay(volatile uint32_t n) {
  while (n != 0U) {
    n--;
  }
}

int main(void) {
  SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;               /* Clock */
  PORTB->PCR[LED_RED_PIN_SHIFT] = PORT_PCR_MUX(1U); /* GPIO */
  GPIOB->PDDR |= LED_RED_PIN_MASK;                  /* Output */
  uart0_init(115200U);
  uart0_puts("TamgaOS boot!\n");

  while (1) {
    GPIOB->PTOR = LED_RED_PIN_MASK;
    delay(LED_DELAY_COUNT);
    uart0_puts("TamgaOS tick!\n");
  }
}
