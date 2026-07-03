/*
 * main.c — STM32H753ZI Nucleo-144 LED blink @ 480MHz
 *
 * LD1 (green)  → PB0
 * LD2 (yellow) → PE1
 * LD3 (red)    → PB14
 */

#include "rcc.h"
#include "systick.h"
#include <stdint.h>

#define RCC_AHB4ENR (*(volatile uint32_t *)0x580244E0U)

#define GPIOB_MODER (*(volatile uint32_t *)0x58020400U)
#define GPIOB_ODR   (*(volatile uint32_t *)0x58020414U)
#define GPIOE_MODER (*(volatile uint32_t *)0x58021000U)
#define GPIOE_ODR   (*(volatile uint32_t *)0x58021014U)

#define LD1_PIN (1UL << 0U)
#define LD2_PIN (1UL << 1U)
#define LD3_PIN (1UL << 14U)

static void board_init(void)
{
    RCC_AHB4ENR |= (1UL << 1U) | (1UL << 4U);
    GPIOB_MODER &= ~((3UL << (0U  * 2U)) | (3UL << (14U * 2U)));
    GPIOB_MODER |=   (1UL << (0U  * 2U)) | (1UL << (14U * 2U));
    GPIOE_MODER &= ~(3UL << (1U * 2U));
    GPIOE_MODER |=   1UL << (1U * 2U);
    GPIOB_ODR &= ~(LD1_PIN | LD3_PIN);
    GPIOE_ODR &= ~LD2_PIN;
}

int main(void)
{
    //rcc_init_hsi64();
    //systick_init(64000000U);
    rcc_init_pll_480();
    systick_init(480000000U);
    board_init();

    while (1) {
        GPIOB_ODR |=  LD1_PIN;
        systick_delay_ms(500U);
        GPIOB_ODR &= ~LD1_PIN;

        GPIOE_ODR |=  LD2_PIN;
        systick_delay_ms(500U);
        GPIOE_ODR &= ~LD2_PIN;

        GPIOB_ODR |=  LD3_PIN;
        systick_delay_ms(500U);
        GPIOB_ODR &= ~LD3_PIN;

        systick_delay_ms(500U);
    }
}