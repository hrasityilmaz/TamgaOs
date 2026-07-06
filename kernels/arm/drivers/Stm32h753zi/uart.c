/*
 * USART3 — Virtual COM port via ST-Link
 * TX → PD8 AF7
 * RX → PD9 AF7
 *
 * Clock tree:
 *   sysclk = 480MHz
 *   AHB    = 240MHz (HPRE=/2)
 *   APB1   = 240MHz (D2PPRE1=/1, default — RCC_D2CFGR not touched)
 *
 * BRR = APB1_clock / baud = 240000000 / 115200 = 2083
 *
 * REF: RM0433 Rev8
 *   USART3 base:  0x40004800
 *   RCC_APB1LENR: 0x58024400 + 0x0E8, USART3EN bit18
 *   GPIOD base:   0x58020C00
 *   RCC_AHB4ENR:  0x58024400 + 0x0E0, GPIODEN bit3
 */

 #include "uart.h"
 #include <stdint.h>

 #define USART3_BASE     0x40004800UL
#define USART3_CR1      (*(volatile uint32_t *)(USART3_BASE + 0x000U))
#define USART3_CR2      (*(volatile uint32_t *)(USART3_BASE + 0x004U))
#define USART3_CR3      (*(volatile uint32_t *)(USART3_BASE + 0x008U))
#define USART3_BRR      (*(volatile uint32_t *)(USART3_BASE + 0x00CU))
#define USART3_ISR      (*(volatile uint32_t *)(USART3_BASE + 0x01CU))
#define USART3_RDR      (*(volatile uint32_t *)(USART3_BASE + 0x024U))
#define USART3_TDR      (*(volatile uint32_t *)(USART3_BASE + 0x028U))

#define USART_CR1_UE (1UL << 0U)
#define USART_CR1_RE (1UL << 2U)
#define USART_CR1_TE (1UL << 3U)

/* USART_ISR bits */
#define USART_ISR_RXNE  (1UL << 5U)    /* Read data register */
#define USART_ISR_TC    (1UL << 6U)    /* Transmission complete */
#define USART_ISR_TXE   (1UL << 7U)    /* Transmit data register*/

/* ── RCC ── */
#define RCC_BASE        0x58024400UL
#define RCC_AHB4ENR     (*(volatile uint32_t *)(RCC_BASE + 0x0E0U))
#define RCC_APB1LENR    (*(volatile uint32_t *)(RCC_BASE + 0x0E8U))

#define RCC_AHB4ENR_GPIODEN   (1UL << 3U)    /* GPIOD clock enable */
#define RCC_APB1LENR_USART3EN (1UL << 18U)   /* USART3 clock enable */

/* ── GPIOD ── */
#define GPIOD_BASE      0x58020C00UL
#define GPIOD_MODER     (*(volatile uint32_t *)(GPIOD_BASE + 0x00U))
#define GPIOD_OSPEEDR   (*(volatile uint32_t *)(GPIOD_BASE + 0x08U))
#define GPIOD_PUPDR     (*(volatile uint32_t *)(GPIOD_BASE + 0x0CU))
#define GPIOD_AFRH      (*(volatile uint32_t *)(GPIOD_BASE + 0x24U))

void uart_init(void){
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIODEN;
    RCC_APB1LENR |= RCC_APB1LENR_USART3EN;
    // Every pin 2bit!!! PD8 8*2 PD9 9x2
    GPIOD_MODER &= ~((3UL << (8U * 2U)) | (3UL << (9U * 2U)));
    GPIOD_MODER |=   (2UL << (8U * 2U)) | (2UL << (9U * 2U));
    GPIOD_OSPEEDR |= (3UL << (8U * 2U)) | (3UL << (9U * 2U));
    GPIOD_PUPDR &= ~((3UL << (8U * 2U)) | (3UL << (9U * 2U)));

    GPIOD_AFRH &= ~((0xFUL << 0U) | (0xFUL << 4U));
    GPIOD_AFRH |=   (7UL   << 0U) | (7UL   << 4U);   /* AF7 */

    USART3_CR1 = 0U;
    USART3_CR2 = 0U;
    USART3_CR3 = 0U;

    // 240000000/115200 = 2083.3 !!
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
    while (*s) {
        uart_putc(*s++);
    }
}

char uart_getc(void)
{
    while ((USART3_ISR & USART_ISR_RXNE) == 0U) {}
    return (char)(USART3_RDR & 0xFFU);
}