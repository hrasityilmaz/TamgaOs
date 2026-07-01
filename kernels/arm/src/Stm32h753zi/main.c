/*
 * main.c — STM32H753ZI Nucleo-144 LED blink test
 *
 * LD1 (green)  → PB0
 * LD2 (yellow) → PE1
 * LD3 (red)    → PB14
 *
 * LEDs ON when pin HIGH.
 *
 * REF: RM0433
 *   RCC_AHB4ENR  0x580244E0 — GPIOBEN, GPIOEEN clock enable
 *   GPIOB_MODER  0x58020400
 *   GPIOB_ODR    0x58020414
 *   GPIOE_MODER  0x58021000
 *   GPIOE_ODR    0x58021014
 */

#include "rcc.h"
#include "systick.h"
#include <stdint.h>

/* ── RCC ── */
#define RCC_AHB4ENR (*(volatile uint32_t *)0x580244E0U)
#define RCC_AHB4ENR_GPIOBEN (1UL << 1U)
#define RCC_AHB4ENR_GPIOEEN (1UL << 4U)

/* ── GPIOB ── */
#define GPIOB_BASE 0x58020400UL
#define GPIOB_MODER (*(volatile uint32_t *)(GPIOB_BASE + 0x00U))
#define GPIOB_ODR (*(volatile uint32_t *)(GPIOB_BASE + 0x14U))

/* ── GPIOE ── */
#define GPIOE_BASE 0x58021000UL
#define GPIOE_MODER (*(volatile uint32_t *)(GPIOE_BASE + 0x00U))
#define GPIOE_ODR (*(volatile uint32_t *)(GPIOE_BASE + 0x14U))

/* ── Pin masks ── */
#define LD1_PIN (1UL << 0U)  /* PB0  */
#define LD2_PIN (1UL << 1U)  /* PE1  */
#define LD3_PIN (1UL << 14U) /* PB14 */

static void board_init(void) {
  /* GPIOB, GPIOE  */
  RCC_AHB4ENR |= RCC_AHB4ENR_GPIOBEN | RCC_AHB4ENR_GPIOEEN;

  /* GPIOB MODER — PB0 output, PB14 output */
  GPIOB_MODER &= ~((3UL << (0U * 2U)) | (3UL << (14U * 2U)));
  GPIOB_MODER |= (1UL << (0U * 2U)) | (1UL << (14U * 2U));

  GPIOE_MODER &= ~(3UL << (1U * 2U));
  GPIOE_MODER |= 1UL << (1U * 2U);
  GPIOB_ODR &= ~(LD1_PIN | LD3_PIN);
  GPIOE_ODR &= ~LD2_PIN;
}

int main(void) {
  rcc_init_hsi64();
  systick_init(64000000U);
  board_init();

  while (1) {
    GPIOB_ODR |= LD1_PIN;
    systick_delay_ms(50U);
    GPIOB_ODR &= ~LD1_PIN;

    GPIOE_ODR |= LD2_PIN;
    systick_delay_ms(50U);
    GPIOE_ODR &= ~LD2_PIN;

    GPIOB_ODR |= LD3_PIN;
    systick_delay_ms(50U);
    GPIOB_ODR &= ~LD3_PIN;

    systick_delay_ms(50U);
  }
}
