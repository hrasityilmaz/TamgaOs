/*
 * pwm.c — STM32H753ZI TIM2 PWM driver (PA0/D32)
 *
 * Channel : TIM2_CH1 on PA0 (Arduino D32 -> confirmed via UM2407 Table 21: "D32 TIM_C_PWM1 PA0 TIM2_CH1")
 */

#include <stdint.h>
#include "uart.h"

#define TIM2_BASE     0x40000000UL
#define TIM2_CR1      (*(volatile uint32_t *)(TIM2_BASE + 0x00U))
#define TIM2_CCMR1    (*(volatile uint32_t *)(TIM2_BASE + 0x18U))
#define TIM2_CCER     (*(volatile uint32_t *)(TIM2_BASE + 0x20U))
#define TIM2_PSC      (*(volatile uint32_t *)(TIM2_BASE + 0x28U))
#define TIM2_ARR      (*(volatile uint32_t *)(TIM2_BASE + 0x2CU))
#define TIM2_CCR1     (*(volatile uint32_t *)(TIM2_BASE + 0x34U))

#define RCC_BASE       0x58024400UL
#define RCC_AHB4ENR    (*(volatile uint32_t *)(RCC_BASE + 0x0E0U))
#define RCC_APB1LENR   (*(volatile uint32_t *)(RCC_BASE + 0x0E8U))
#define RCC_APB1LENR_TIM2EN_MASK (1UL << 0U)

#define GPIOA_BASE     0x58020000UL
#define GPIOA_MODER    (*(volatile uint32_t *)(GPIOA_BASE + 0x00U))
#define GPIOA_OSPEEDR  (*(volatile uint32_t *)(GPIOA_BASE + 0x08U))
#define GPIOA_AFRL     (*(volatile uint32_t *)(GPIOA_BASE + 0x20U)) 
#define GPIOA_ODR      (*(volatile uint32_t *)(GPIOA_BASE + 0x14U))

#define PWM_AF_TIM2_CH1  (1U) 
#define PWM_TIMER_CLOCK_HZ  240000000UL

void pwm_debug_gpio_toggle_test(void)
{
    RCC_AHB4ENR |= (1UL << 0U); 
    GPIOA_MODER &= ~(3UL << (0U * 2U));
    GPIOA_MODER |=  (1UL << (0U * 2U));

    uint32_t counter = 0U;
    for (;;) {
        GPIOA_ODR ^= (1UL << 0U);
        uart_printf("[TOGGLE-TEST-PA0] tick=%u, GPIOA_ODR=0x%x\r\n",
                    (unsigned int)counter, (unsigned int)GPIOA_ODR);
        counter++;

        for (volatile uint32_t i = 0; i < 20000000U; i++) { }
    }
}

void pwm_init(void)
{
    RCC_AHB4ENR  |= (1UL << 0U);
    RCC_APB1LENR |= RCC_APB1LENR_TIM2EN_MASK;
    GPIOA_MODER &= ~(3UL << (0U * 2U));
    GPIOA_MODER |=  (2UL << (0U * 2U)); 
    GPIOA_OSPEEDR |= (3UL << (0U * 2U));

    GPIOA_AFRL &= ~(0xFUL << (0U * 4U));
    GPIOA_AFRL |=  (PWM_AF_TIM2_CH1 << (0U * 4U));
    TIM2_PSC = (PWM_TIMER_CLOCK_HZ / 1000000UL) - 1UL;
    TIM2_ARR = 20000UL - 1UL;
    TIM2_CCMR1 &= ~(0x7UL << 4U);
    TIM2_CCMR1 |=  (0x6UL << 4U);
    TIM2_CCMR1 |=  (1UL   << 3U); 
    TIM2_CCR1 = 1000UL;
    TIM2_CCER |= (1UL << 0U); 
    TIM2_CR1 |= (1UL << 0U);

    uart_printf("[PWM-TIM2] init done: PSC=%u ARR=%u\r\n",
                (unsigned int)TIM2_PSC, (unsigned int)TIM2_ARR);
}

void pwm_set_pulse_us(uint16_t pulse_us)
{
    if (pulse_us < 1000U) pulse_us = 1000U;
    if (pulse_us > 2000U) pulse_us = 2000U;
    TIM2_CCR1 = pulse_us;
}

// Can be remove or stay for debug...
void pwm_debug_print_regs(void)
{
    uart_printf("[PWM-DEBUG] TIM2_CR1=0x%x TIM2_CCER=0x%x\r\n",
                (unsigned int)TIM2_CR1, (unsigned int)TIM2_CCER);
    uart_printf("[PWM-DEBUG] GPIOA_MODER=0x%x GPIOA_AFRL=0x%x\r\n",
                (unsigned int)GPIOA_MODER, (unsigned int)GPIOA_AFRL);
}