#include "MK64F12REGS.h"
#include "mmio_deviation.h"
#include "uart.h"
#include <stdint.h>

#define CORE_CLOCK_HZ (20971520UL)

#define UART0_SBR_DIVISOR (16U)

#define UART0_TX_PORT_PIN (16U)
#define UART0_RX_PORT_PIN (17U)

void uart0_init(uint32_t baud) {
  // uint16_t sbr;
  // SIM->SCGC4 |= SIM_SCGC4_UART0_MASK;
  // SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
  // SIM->SOPT2 &= ~SIM_SOPT2_UART0SRC_MASK;
  // SIM->SOPT2 |= SIM_SOPT2_UART0SRC(SIM_SOPT2_UART0SRC_MCGFLL);
  //
  // PORTB->PCR[UART0_TX_PORT_PIN] = PORT_PCR_MUX(3U);
  // PORTB->PCR[UART0_RX_PORT_PIN] = PORT_PCR_MUX(3U);
  // UART0->C2 = 0U;
  //
  // sbr = (uint16_t)(CORE_CLOCK_HZ / (UART0_SBR_DIVISOR * baud));
  //
  // UART0->BDH = (uint8_t)((sbr >> 8U) & UART_BDH_SBR_MASK);
  // UART0->BDL = (uint8_t)(sbr & 0xFFU);
  //
  // UART0->C1 = 0U;
  // UART0->C4 = 0U;
  // UART0->C2 = UART_C2_TE_MASK | UART_C2_RE_MASK;

  uint32_t sbr_full;
  uint16_t sbr;
  uint8_t brfa;

  SIM->SCGC4 |= SIM_SCGC4_UART0_MASK;
  SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
  SIM->SOPT2 &= ~SIM_SOPT2_UART0SRC_MASK;
  SIM->SOPT2 |= SIM_SOPT2_UART0SRC(SIM_SOPT2_UART0SRC_MCGFLL);
  PORTB->PCR[UART0_TX_PORT_PIN] = PORT_PCR_MUX(3U);
  PORTB->PCR[UART0_RX_PORT_PIN] = PORT_PCR_MUX(3U);
  UART0->C2 = 0U;

  sbr_full = (CORE_CLOCK_HZ * 2UL) / baud;
  sbr = (uint16_t)(sbr_full >> 5U);
  brfa = (uint8_t)(sbr_full & UART_C4_BRFA_MASK);
  UART0->BDH = (uint8_t)((sbr >> 8U) & UART_BDH_SBR_MASK);
  UART0->BDL = (uint8_t)(sbr & 0xFFU);
  UART0->C1 = 0U;
  UART0->C4 = brfa;
  UART0->C2 = UART_C2_TE_MASK | UART_C2_RE_MASK;
}

void uart0_putc(char c) {
  while ((UART0->S1 & UART_S1_TDRE_MASK) == 0U) {
    /* wait */
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
