/*
 * USART3 — Virtual COM port via ST-Link
 * TX → PD8 AF7
 * RX → PD9 AF7
 *
 * APB1 = 240MHz, BRR = 2083
 */

#include "uart.h"
#include <stdarg.h>
#include <stdint.h>

#define USART3_BASE  0x40004800UL
#define USART3_CR1   (*(volatile uint32_t *)(USART3_BASE + 0x000U))
#define USART3_CR2   (*(volatile uint32_t *)(USART3_BASE + 0x004U))
#define USART3_CR3   (*(volatile uint32_t *)(USART3_BASE + 0x008U))
#define USART3_BRR   (*(volatile uint32_t *)(USART3_BASE + 0x00CU))
#define USART3_ISR   (*(volatile uint32_t *)(USART3_BASE + 0x01CU))
#define USART3_RDR   (*(volatile uint32_t *)(USART3_BASE + 0x024U))
#define USART3_TDR   (*(volatile uint32_t *)(USART3_BASE + 0x028U))

#define USART_CR1_UE  (1UL << 0U)
#define USART_CR1_RE  (1UL << 2U)
#define USART_CR1_TE  (1UL << 3U)
#define USART_ISR_RXNE (1UL << 5U)
#define USART_ISR_TXE  (1UL << 7U)

#define RCC_BASE      0x58024400UL
#define RCC_AHB4ENR   (*(volatile uint32_t *)(RCC_BASE + 0x0E0U))
#define RCC_APB1LENR  (*(volatile uint32_t *)(RCC_BASE + 0x0E8U))

#define GPIOD_BASE    0x58020C00UL
#define GPIOD_MODER   (*(volatile uint32_t *)(GPIOD_BASE + 0x00U))
#define GPIOD_OSPEEDR (*(volatile uint32_t *)(GPIOD_BASE + 0x08U))
#define GPIOD_PUPDR   (*(volatile uint32_t *)(GPIOD_BASE + 0x0CU))
#define GPIOD_AFRH    (*(volatile uint32_t *)(GPIOD_BASE + 0x24U))

void uart_init(void)
{
    RCC_AHB4ENR  |= (1UL << 3U);
    RCC_APB1LENR |= (1UL << 18U);

    GPIOD_MODER   &= ~((3UL << (8U*2U)) | (3UL << (9U*2U)));
    GPIOD_MODER   |=   (2UL << (8U*2U)) | (2UL << (9U*2U));
    GPIOD_OSPEEDR |=   (3UL << (8U*2U)) | (3UL << (9U*2U));
    GPIOD_PUPDR   &= ~((3UL << (8U*2U)) | (3UL << (9U*2U)));
    GPIOD_AFRH    &= ~((0xFUL << 0U) | (0xFUL << 4U));
    GPIOD_AFRH    |=   (7UL << 0U) | (7UL << 4U);

    USART3_CR1 = 0U;
    USART3_CR2 = 0U;
    USART3_CR3 = 0U;
    USART3_BRR = 2083U;
    USART3_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void uart_putc(char c)
{
    while ((USART3_ISR & USART_ISR_TXE) == 0U) {}
    USART3_TDR = (uint32_t)c;
}

void uart_puts(const char *s)
{
    while (*s) uart_putc(*s++);
}

char uart_getc(void)
{
    while ((USART3_ISR & USART_ISR_RXNE) == 0U) {}
    return (char)(USART3_RDR & 0xFFU);
}

static void print_uint(uint32_t n, uint8_t base)
{
    char buf[32];
    int  i = 31;
    buf[i] = '\0';
    if (n == 0U) { uart_putc('0'); return; }
    while (n > 0U && i > 0) {
        uint32_t rem = n % base;
        buf[--i] = (char)(rem < 10U ? '0' + rem : 'a' + rem - 10U);
        n /= base;
    }
    uart_puts(&buf[i]);
}

static void print_int(int32_t n)
{
    if (n < 0) { uart_putc('-'); n = -n; }
    print_uint((uint32_t)n, 10U);
}

void uart_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    while (*fmt) {
        if (*fmt != '%') { uart_putc(*fmt++); continue; }
        fmt++;
        switch (*fmt) {
            case 'd': print_int(va_arg(args, int));             break;
            case 'u': print_uint(va_arg(args, uint32_t), 10U); break;
            case 'x': print_uint(va_arg(args, uint32_t), 16U); break;
            case 's': uart_puts(va_arg(args, const char *));   break;
            case 'c': uart_putc((char)va_arg(args, int));      break;
            case '%': uart_putc('%');                           break;
            default:  uart_putc('%'); uart_putc(*fmt);         break;
        }
        fmt++;
    }
    va_end(args);
}