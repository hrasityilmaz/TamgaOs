/*
 * tests/test_tickless_idle.c — verifies that a task delayed via
 * sched_delay_ms() actually wakes up after approximately the
 * requested duration, even when tickless idle causes SysTick to
 * sleep for long stretches instead of ticking every 1ms.
 */

#include "mcg.h"
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
    mcg_init_120mhz();
    systick_init(120000000UL);
    uart_init(115200U);

    uart_puts("TamgaOS K64F — Tickless Idle Test\r\n\r\n");

    sched_init();
    sched_task_create(test_task_func, 1U);
    sched_start();

    for (;;) { }
    return 0;
}