/*
 * tests/test_jitter_monitor.c
 */

#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "scheduler.h"
#include "jitter_monitor.h"

#define PERIOD_MS   20U
#define CYCLES      40U

static void simulate_work(uint32_t iterations)
{
    for (volatile uint32_t i = 0; i < iterations; i++) { }
}

static void steady_task(void)
{
    jitter_handle_t h = jitter_monitor_create("steady", PERIOD_MS);

    for (uint32_t cycle = 0U; cycle < CYCLES; cycle++) {
        jitter_monitor_mark(h);
        simulate_work(50000U); 
        sched_delay_ms(PERIOD_MS);
    }

    jitter_monitor_report(h);

    for (;;) {
        sched_delay_ms(1000U);
    }
}

static void irregular_task(void)
{
    jitter_handle_t h = jitter_monitor_create("irregular", PERIOD_MS);

    for (uint32_t cycle = 0U; cycle < CYCLES; cycle++) {
        jitter_monitor_mark(h);
        if ((cycle % 5U) == 4U) {
            simulate_work(2000000U);
        } else {
            simulate_work(50000U);
        }

        sched_delay_ms(PERIOD_MS);
    }

    jitter_monitor_report(h);

    for (;;) {
        sched_delay_ms(1000U);
    }
}

static void reporter_task(void)
{
    sched_delay_ms(3000U);

    uart_puts("\r\n=== Final Jitter Report (both tasks) ===\r\n");
    jitter_monitor_report_all();
    uart_puts("=== Done ===\r\n");

    for (;;) {
        sched_delay_ms(1000U);
    }
}

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();

    uart_puts("TamgaOS STM32H753ZI — Jitter Monitor Test\r\n");
    uart_printf("Two tasks, both targeting %ums period, %u cycles each\r\n\r\n",
                (unsigned int)PERIOD_MS, (unsigned int)CYCLES);

    sched_init();
    sched_task_create(steady_task, 1U);
    sched_task_create(irregular_task, 1U);
    sched_task_create(reporter_task, 2U);
    sched_start();

    for (;;) { }
    return 0;
}