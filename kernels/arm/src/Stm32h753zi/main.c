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

/* Task A — float accumulates up, should always increase */
void task_float_a(void)
{
    float val = 0.0f;
    while (1) {
        val += 0.1f;
        GPIOB_ODR ^= LD1_PIN;
        sched_delay_ms(500U);
        int i = (int)val;
        int f = (int)((val - (float)i) * 1000.0f);
        if (f < 0) f = -f;
        uart_printf("[A] %d.%d\r\n", i, f);
    }
}

/* Task B — float decreases down, should always decrease */
void task_float_b(void)
{
    float val = 100.0f;
    while (1) {
        val -= 0.3f;
        GPIOB_ODR ^= LD3_PIN;
        sched_delay_ms(700U);
        int i = (int)val;
        int f = (int)((val - (float)i) * 1000.0f);
        if (f < 0) f = -f;
        uart_printf("[B] %d.%d\r\n", i, f);
    }
}

/* Task LED — no float */
void task_led_yellow(void)
{
    while (1) {
        GPIOE_ODR ^= LD2_PIN;
        sched_delay_ms(250U);
    }
}

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    board_init();
    uart_init();

    uart_puts("TamgaOS STM32H753ZI @ 480MHz\r\n");
    uart_puts("FPU context isolation test\r\n");

    sched_init();

    sched_task_create(task_float_a,    TASK_PRIORITY_NORMAL);
    sched_task_create(task_float_b,    TASK_PRIORITY_NORMAL);
    sched_task_create(task_led_yellow, TASK_PRIORITY_NORMAL);

    sched_start();

    return 0;
}