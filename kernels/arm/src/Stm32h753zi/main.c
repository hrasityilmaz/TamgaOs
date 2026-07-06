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

#define LD1_PIN (1UL << 0U)    /* PB0  green  */
#define LD2_PIN (1UL << 1U)    /* PE1  yellow */
#define LD3_PIN (1UL << 14U)   /* PB14 red    */

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

static void task_led_green(void)
{
    while (1) {
        GPIOB_ODR |=  LD1_PIN;
        sched_delay_ms(50U);
        GPIOB_ODR &= ~LD1_PIN;
        sched_delay_ms(50U);
    }
}

static void task_led_yellow(void)
{
    while (1) {
        GPIOE_ODR |=  LD2_PIN;
        sched_delay_ms(50U);
        GPIOE_ODR &= ~LD2_PIN;
        sched_delay_ms(50U);
    }
}

static void task_led_red(void)
{
    while (1) {
        GPIOB_ODR |=  LD3_PIN;
        sched_delay_ms(50U);
        GPIOB_ODR &= ~LD3_PIN;
        sched_delay_ms(50U);
    }
}

static void task_uart_test(void){
    for(;;){
        uart_puts("[HIGH] task test\r\n");
        sched_delay_ms(1200U);
    }
}

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    board_init();
    uart_init();

    uart_puts("TamgaOS STM32H753ZI @ 480MHz\r\n");
    uart_puts("3 LED blink and 1 UART @ 50ms independent\r\n");

    sched_init();

    sched_task_create(task_led_green,  TASK_PRIORITY_NORMAL);
    sched_task_create(task_led_yellow, TASK_PRIORITY_NORMAL);
    sched_task_create(task_led_red,    TASK_PRIORITY_NORMAL);
    sched_task_create(task_uart_test, TASK_PRIORITY_LOW);

    //systick_sched_enable();
    sched_start();

    return 0;
}