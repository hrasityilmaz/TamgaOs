/*
 * tests/test_stack_monitor.c
 */

#include "mcg.h"
#include "systick.h"
#include "uart.h"
#include "scheduler.h"
#include "stack_monitor.h"

extern task_t *volatile g_current_task;

static task_t *volatile s_shallow_handle = 0;
static task_t *volatile s_deep_handle    = 0;

static void consume_stack(uint32_t depth)
{
    volatile uint8_t buf[32];
    for (uint8_t i = 0U; i < 32U; i++) { buf[i] = (uint8_t)i; }
    if (depth > 0U) {
        consume_stack(depth - 1U);
    }
}

static void shallow_task(void)
{
    s_shallow_handle = g_current_task;
    consume_stack(2U);
    for (;;) {
        sched_delay_ms(1000U);
    }
}

static void deep_task(void)
{
    s_deep_handle = g_current_task;
    consume_stack(15U); 
    for (;;) {
        sched_delay_ms(1000U);
    }
}

static void reporter_task(void)
{
    sched_delay_ms(500U);

    uint32_t shallow_used = stack_monitor_get_used_bytes(s_shallow_handle);
    uint32_t deep_used    = stack_monitor_get_used_bytes(s_deep_handle);
    uint8_t  shallow_pct  = stack_monitor_get_used_percent(s_shallow_handle);
    uint8_t  deep_pct     = stack_monitor_get_used_percent(s_deep_handle);

    uart_printf("[STACK] shallow_task used=%u bytes (%u%%)\r\n",
                (unsigned int)shallow_used, (unsigned int)shallow_pct);
    uart_printf("[STACK] deep_task    used=%u bytes (%u%%)\r\n",
                (unsigned int)deep_used, (unsigned int)deep_pct);

    if (deep_used > shallow_used) {
        uart_puts("[TEST] PASS: deep_task used more stack than shallow_task, as expected\r\n");
    } else {
        uart_puts("[TEST] FAIL: expected deep_task to use more stack than shallow_task\r\n");
    }

    uart_puts("=== Done ===\r\n");
    for (;;) { }
}

int main(void)
{
    mcg_init_120mhz();
    systick_init(120000000UL);
    uart_init(115200U);
    uart_puts("TamgaOS K64F — Stack High-Water Mark Test\r\n\r\n");

    sched_init();
    sched_task_create(shallow_task, 1U);
    sched_task_create(deep_task, 1U);
    sched_task_create(reporter_task, 2U);
    sched_start();

    for (;;) { }
    return 0;
}