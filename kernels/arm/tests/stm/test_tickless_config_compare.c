/*
 * tests/test_tickless_config_compare.c -- demonstrates the measurable difference what tickless idle makes
 */

#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "scheduler.h"
#include "tamgaos_config.h" 

static volatile uint32_t s_idle_iterations = 0U;


static void worker_task(void)
{
    for (uint8_t i = 0U; i < 5U; i++) {
        uint32_t before = systick_get_ms();
        sched_delay_ms(500U);
        uint32_t after = systick_get_ms();

        uart_printf("[DELAY] cycle=%u requested=500ms actual=%ums idle_loops_so_far=%u\r\n",
                    (unsigned int)i, (unsigned int)(after - before),
                    (unsigned int)sched_get_idle_loop_count());
    }

    uart_printf("\r\n[RESULT] total idle_loop_count after ~2.5s = %u\r\n",
                (unsigned int)sched_get_idle_loop_count());
    uart_puts("=== Done ===\r\n");
    for (;;) { }
}

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();

#if TAMGAOS_TICKLESS_IDLE_DEFAULT
    uart_puts("TamgaOS STM32H753ZI — Tickless Compare [ON]\r\n\r\n");
#else
    uart_puts("TamgaOS STM32H753ZI — Tickless Compare [OFF]\r\n\r\n");
#endif

    sched_init();
    sched_task_create(worker_task, 1U);
    sched_start();

    for (;;) { }
    return 0;
}