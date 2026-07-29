#include "mmio_deviation.h"
#include "system_clock.h"
#include "uart.h"
#include <stdint.h>
#include <stdarg.h>

#define UART_TX_BUF_SIZE (256U)
static volatile uint8_t  s_tx_buf[UART_TX_BUF_SIZE];
static volatile uint32_t s_tx_head;
static volatile uint32_t s_tx_tail;

#define UART_RX_BUF_SIZE (64U)
static volatile uint8_t  s_rx_buf[UART_RX_BUF_SIZE];
static volatile uint32_t s_rx_head;
static volatile uint32_t s_rx_tail;

/*
 * UART_S1_RDRF_MASK — NOT present in mmio_deviation.h at the time
 * this was written. Bit position taken from the K64 Sub-Family
 * Reference Manual's UART0_S1 register table (RDRF = bit 5), the
 * same register this driver already reads TDRE (bit 7) from. If
 * mmio_deviation.h later adds its own UART_S1_RDRF_MASK, prefer that
 * one and remove this local definition to avoid a conflicting
 * redefinition.
 */
#ifndef UART_S1_RDRF_MASK
#define UART_S1_RDRF_MASK  (1UL << 5U)
#endif

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
  s_rx_head = 0U;
  s_rx_tail = 0U;

  UART0->C2 = UART_C2_TE_MASK | UART_C2_RE_MASK | (1UL << 5U);  /* + RIE, RX interrupt enable */
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

  if (UART0->S1 & UART_S1_RDRF_MASK) {
    uint8_t byte = UART0->D;
    uint32_t next_head = (s_rx_head + 1U) % UART_RX_BUF_SIZE;
    if (next_head != s_rx_tail) {   /* drop the byte if the ring is full */
      s_rx_buf[s_rx_head] = byte;
      s_rx_head = next_head;
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

char uart_getc(void) {
  while (s_rx_head == s_rx_tail) { }
  char c = (char)s_rx_buf[s_rx_tail];
  s_rx_tail = (s_rx_tail + 1U) % UART_RX_BUF_SIZE;
  return c;
}

static void uart_put_uint(uint32_t val, uint8_t base, uint8_t uppercase)
{
    char buf[12];
    int i = 0;

    if (val == 0U) {
        uart_putc('0');
        return;
    }

    while (val > 0U) {
        uint32_t digit = val % base;
        if (digit < 10U) {
            buf[i++] = (char)('0' + digit);
        } else {
            buf[i++] = (char)((uppercase ? 'A' : 'a') + (digit - 10U));
        }
        val /= base;
    }

    while (i > 0) {
        uart_putc(buf[--i]);
    }
}

static void uart_put_int(int32_t val)
{
    if (val < 0) {
        uart_putc('-');
        uart_put_uint((uint32_t)(-val), 10U, 0U);
    } else {
        uart_put_uint((uint32_t)val, 10U, 0U);
    }
}

void uart_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    while (*fmt != '\0') {
        if (*fmt != '%') {
            if (*fmt == '\n') uart_putc('\r');
            uart_putc(*fmt);
            fmt++;
            continue;
        }

        fmt++;
        switch (*fmt) {
            case 'u':
                uart_put_uint(va_arg(args, uint32_t), 10U, 0U);
                break;
            case 'd': {
                int32_t v = va_arg(args, int32_t);
                uart_put_int(v);
                break;
            }
            case 'x':
                uart_put_uint(va_arg(args, uint32_t), 16U, 0U);
                break;
            case 'X':
                uart_put_uint(va_arg(args, uint32_t), 16U, 1U);
                break;
            case 's': {
                const char *s = va_arg(args, const char *);
                uart_puts(s);
                break;
            }
            case 'c': {
                char c = (char)va_arg(args, int);
                uart_putc(c);
                break;
            }
            case '%':
                uart_putc('%');
                break;
            default:
                uart_putc('%');
                uart_putc(*fmt);
                break;
        }
        fmt++;
    }

    va_end(args);
}