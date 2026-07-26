/*
 * tests/test_deadline_monitor.c — simulates a flight-control-style
 * periodic task (fixed budget, e.g. 10ms per cycle)
 *
 * simulate_work() busy-waits for a requested number of CPU cycles
 * using the Cortex-M7 DWT cycle counter (DWT->CYCCNT)
 */

#include <stdint.h>
#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "deadline_monitor.h"

#define FLIGHT_TASK_MAX_MS   10U
#define TEST_CYCLES          30U
#define CPU_HZ               480000000U

/* Convert a millisecond duration to CPU cycles at CPU_HZ. */
#define MS_TO_CYCLES(ms)     ((uint32_t)(((uint64_t)(ms) * CPU_HZ) / 1000U))
#define REG32(addr)          (*(volatile uint32_t *)(addr))
#define CM7_DEMCR_ADDR       0xE000EDFCUL
#define CM7_DEMCR_TRCENA_BIT (1UL << 24)
#define CM7_DWT_CTRL_ADDR    0xE0001000UL
#define CM7_DWT_CTRL_CYCCNTENA_BIT (1UL << 0)
#define CM7_DWT_CYCCNT_ADDR  0xE0001004UL

static void dwt_cycle_counter_init(void)
{
    REG32(CM7_DEMCR_ADDR) |= CM7_DEMCR_TRCENA_BIT;
    REG32(CM7_DWT_CYCCNT_ADDR) = 0U;
    REG32(CM7_DWT_CTRL_ADDR)  |= CM7_DWT_CTRL_CYCCNTENA_BIT;
}

static inline uint32_t dwt_cyccnt(void)
{
    return REG32(CM7_DWT_CYCCNT_ADDR);
}

static void busy_wait_cycles(uint32_t cycles)
{
    uint32_t start = dwt_cyccnt();
    while ((dwt_cyccnt() - start) < cycles) {}
}

static void simulate_work(uint32_t work_ms)
{
    busy_wait_cycles(MS_TO_CYCLES(work_ms));
}

int main(void)
{
    rcc_init_pll_480();
    systick_init(CPU_HZ);
    dwt_cycle_counter_init();
    uart_init();

    uart_puts("TamgaOS STM32H753ZI — Deadline/Response Time Monitor Test\r\n");
    uart_printf("Simulated flight task, max=%ums, %u cycles\r\n\r\n",
                (unsigned int)FLIGHT_TASK_MAX_MS, (unsigned int)TEST_CYCLES);

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