#include "mmio_deviation.h"
#include "system_clock.h"
#include "uart.h"
#include <stdint.h>

#define UART_TX_BUF_SIZE (256U)
static volatile uint8_t  s_tx_buf[UART_TX_BUF_SIZE];
static volatile uint32_t s_tx_head;
static volatile uint32_t s_tx_tail;

void uart_init(uint32_t baud) {
  SIM->SCGC4 |= SIM_SCGC4_UART0_MASK;
  SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
  SIM->SOPT2 = (SIM->SOPT2 & ~SIM_SOPT2_UART0SRC_MASK) |
               SIM_SOPT2_UART0SRC(SIM_SOPT2_UART0SRC_MCGFLL);
  PORTB->PCR[16] = PORT_PCR_MUX(3U);
  PORTB->PCR[17] = PORT_PCR_MUX(3U);
  UART0->C2 = 0U;
  UART0->BDH = 0U;
  UART0->BDL = (uint8_t)(UART0_CLOCK_HZ / (16UL * baud));
  UART0->C4  = 0U;
  UART0->C1  = 0U;
  s_tx_head = 0U;
  s_tx_tail = 0U;

  UART0->C2 = UART_C2_TE_MASK | UART_C2_RE_MASK;
  volatile uint8_t *nvic_ipr = (volatile uint8_t *)0xE000E400UL;
  nvic_ipr[31] = 0x80U;
  volatile uint32_t *nvic_iser = (volatile uint32_t *)0xE000E100UL;
  nvic_iser[0] |= (1UL << 31U);
}

void UART0_RX_TX_IRQHandler(void) {
  if (UART0->S1 & UART_S1_TDRE_MASK) {
    if (s_tx_tail != s_tx_head) {
      UART0->D = s_tx_buf[s_tx_tail];
      s_tx_tail = (s_tx_tail + 1U) % UART_TX_BUF_SIZE;
    } else {
      UART0->C2 &= ~(1U << 7U);
    }
  }
}

void uart_putc(char c) {
  uint32_t next_head = (s_tx_head + 1U) % UART_TX_BUF_SIZE;
  while (next_head == s_tx_tail) {}
  s_tx_buf[s_tx_head] = (uint8_t)c;
  s_tx_head = next_head;
  UART0->C2 |= (1U << 7U);
}

void uart_puts(const char *s) {
  while (*s != '\0') {
    if (*s == '\n') uart_putc('\r');
    uart_putc(*s);
    s++;
  }
}