/*
 * tests/k64f/test_deadline_monitor.c 
 */

#include "mcg.h"
#include "systick.h"
#include "uart.h"
#include "deadline_monitor.h"

#define FLIGHT_TASK_MAX_MS  10U
#define TEST_CYCLES            30U

static void simulate_work(uint32_t iterations)
{
    for (volatile uint32_t i = 0; i < iterations; i++) { }
}

int main(void)
{
    mcg_init_120mhz();
    systick_init(120000000UL);
    uart_init(115200U);

    uart_puts("TamgaOS K64F — Deadline/Response Time Monitor Test\r\n");
    uart_printf("Simulated flight task, max=%ums, %u cycles\r\n\r\n",
                (unsigned int)FLIGHT_TASK_BUDGET_MS, (unsigned int)TEST_CYCLES);

     monitor_handle_t flight_task = deadline_monitor_create("flight_ctrl", FLIGHT_TASK_MAX_MS);
    if (flight_task < 0) {
        uart_puts("FAIL: deadline_monitor_create returned -1\r\n");
        for (;;) { }
    }

    for (uint32_t cycle = 0U; cycle < TEST_CYCLES; cycle++) {
        deadline_monitor_begin(flight_task);

        if ((cycle % 7U) == 6U) {
            simulate_work(14U); /* 14ms of work. */
        } else {
            simulate_work(4U);  /* 4ms of work */
        }

        uint8_t overran = deadline_monitor_end(flight_task);
        if (overran) {
            uart_printf("[ALERT] cycle %u exceeded budget!\r\n", (unsigned int)cycle);
        }

        systick_delay_ms(50U); /* wait a little */
    }

    uart_puts("\r\n=== Final Report ===\r\n");
    deadline_monitor_report_all();

    for (;;) { }
    return 0;
}