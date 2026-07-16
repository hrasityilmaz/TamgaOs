/*
 * test_mpu_stack_guard.c — triggers the MPU stack-overflow guard
 * for real, by running INSIDE the scheduler .
 */

#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "fault_log.h"
#include "scheduler.h"
#include "task.h"
#include <stdint.h>

static void report_previous_fault(void)
{
    fault_log_t f;
    fault_log_init();

    if (fault_log_check_and_clear(&f)) {
        uart_puts("=== Previous boot ended in a fault ===\r\n");
        uart_printf("EXC_RETURN=0x%x PC=0x%x LR=0x%x xPSR=0x%x\r\n",
                    f.exc_return, f.pc, f.lr, f.xpsr);
        uart_printf("CFSR=0x%x HFSR=0x%x\r\n", f.cfsr, f.hfsr);
        if (f.mmfar_valid) uart_printf("MMFAR=0x%x\r\n", f.mmfar);
        if (f.bfar_valid)  uart_printf("BFAR=0x%x\r\n", f.bfar);
        uart_puts("=======================================\r\n\r\n");
    } else {
        uart_puts("No pending fault log (clean boot or first flash)\r\n\r\n");
    }
}

/* Forward-declared so it can call itself */
static void stack_burner_recurse(volatile uint32_t depth);

static void stack_burner_task(void)
{
    uart_puts("Task started — burning its own stack now...\r\n");
    systick_delay_ms(50U);   /* let UART finish before the guard fires */
    stack_burner_recurse(0U);

    /* Unreachable if the MPU guard fired as expected */
    uart_puts("ERROR: recursion returned without hitting the guard\r\n");
    while (1) {}
}

static void stack_burner_recurse(volatile uint32_t depth)
{
    /* Volatile local array forces a real stack frame each call —
       without -O2, GCC won't tail-call-optimize this away, but the
       volatile also blocks it regardless of optimization level. */
    volatile uint32_t pad[16];
    pad[0] = depth;
    stack_burner_recurse(depth + 1U);
}

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();

    uart_puts("TamgaOS STM32H753ZI @ 480MHz\r\n");
    uart_puts("MPU stack guard test\r\n\r\n");

    report_previous_fault();

    sched_init();
    sched_task_create(stack_burner_task, TASK_PRIORITY_HIGH);
    sched_start();   /* never returns */

    while (1) {}
    return 0;
}