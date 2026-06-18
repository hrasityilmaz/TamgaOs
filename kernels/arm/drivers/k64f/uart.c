#include "mmio_deviation.h"
#include "system_clock.h"
#include "uart.h"
#include <stdint.h>

void uart0_init(uint32_t baud) {
  SIM->SCGC4 |= SIM_SCGC4_UART0_MASK;
  SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;

  SIM->SOPT2 = (SIM->SOPT2 & ~SIM_SOPT2_UART0SRC_MASK) |
               SIM_SOPT2_UART0SRC(SIM_SOPT2_UART0SRC_MCGFLL);

  PORTB->PCR[16] = PORT_PCR_MUX(3U);
  PORTB->PCR[17] = PORT_PCR_MUX(3U);

  UART0->C2 = 0U;

  UART0->BDH = 0U;
  UART0->BDL = (uint8_t)(UART0_CLOCK_HZ / (16L * baud));
  UART0->C4 = 0U;

  UART0->C1 = 0U;
  UART0->C2 = UART_C2_TE_MASK | UART_C2_RE_MASK;
}

void uart0_putc(char c) {
  while ((UART0->S1 & UART_S1_TDRE_MASK) == 0U) {
  }
  UART0->D = (uint8_t)c;
}

void uart0_puts(const char *s) {
  while (*s != '\0') {
    if (*s == '\n') {
      uart0_putc('\r');
    }
    uart0_putc(*s);
    s++;
  }
}
