#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "scheduler.h"

static void test_task_func(void)
{
    for (;;) {
        uint32_t before = systick_get_ms();
        sched_delay_ms(500U);
        uint32_t after = systick_get_ms();

        uart_printf("[TICKLESS] requested=500ms, actual elapsed=%ums\r\n",
                    (unsigned int)(after - before));
    }
}

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();

    uart_puts("TamgaOS STM32H753ZI — Tickless Idle Test\r\n\r\n");

    sched_init();
    sched_task_create(test_task_func, 1U);
    sched_start();

    for (;;) { }
    return 0;
}