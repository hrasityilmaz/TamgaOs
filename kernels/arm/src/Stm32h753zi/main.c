#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "scheduler.h"
#include "task.h"
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

void task_led_green(void)
{
    while (1) {
        GPIOB_ODR |=  LD1_PIN;
        sched_delay_ms(500U);
        GPIOB_ODR &= ~LD1_PIN;
        sched_delay_ms(500U);
    }
}

void task_led_yellow(void)
{
    while (1) {
        GPIOE_ODR |=  LD2_PIN;
        sched_delay_ms(300U);
        GPIOE_ODR &= ~LD2_PIN;
        sched_delay_ms(300U);
    }
}

void task_led_red(void)
{
    while (1) {
        GPIOB_ODR |=  LD3_PIN;
        sched_delay_ms(700U);
        GPIOB_ODR &= ~LD3_PIN;
        sched_delay_ms(700U);
    }
}

void task_uart(void)
{
    int cnt = 0;
    while (1) {
        cnt++;
        uart_printf("[UART] tick = %d\r\n", cnt);
        sched_delay_ms(1000U);
    }
}

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    board_init();
    uart_init();

    uart_puts("TamgaOS STM32H753ZI @ 480MHz\r\n");
    uart_puts("FreeRTOS-style context switch test\r\n");

    sched_init();

    sched_task_create(task_led_green,  TASK_PRIORITY_NORMAL);
    sched_task_create(task_led_yellow, TASK_PRIORITY_NORMAL);
    sched_task_create(task_led_red,    TASK_PRIORITY_NORMAL);
    sched_task_create(task_uart,       TASK_PRIORITY_LOW);

    sched_start();

    return 0;
}