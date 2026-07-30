/*
 * tests/k64f/test_task_notify.c 
 */

#include "mcg.h"
#include "systick.h"
#include "uart.h"
#include "scheduler.h"
#include "notify.h"

extern task_t *volatile g_current_task;

static task_t *volatile s_waiter_handle = 0;

static volatile uint8_t  s_isr_notify_armed    = 0U;
static volatile uint32_t s_isr_arm_countdown   = 0U;

static void keepalive_task(void)
{
    for (;;) {
        sched_delay_ms(1U);
    }
}

void systick_isr_hook(void)
{
    if (s_isr_notify_armed) {
        if (s_isr_arm_countdown > 0U) {
            s_isr_arm_countdown--;
        }
        if (s_isr_arm_countdown == 0U) {
            if (s_waiter_handle != 0) {
                task_notify_give_from_isr(s_waiter_handle, 0xABCDU);
            }
            s_isr_notify_armed = 0U;
        }
    }
}

static void waiter_task(void)
{
    s_waiter_handle = g_current_task;

    uart_puts("[TEST 1] give-then-wait: notifying self before waiting\r\n");
    task_notify_give(s_waiter_handle, 111U);
    uint32_t value = 0U;
    uint8_t got = task_notify_wait(&value, 1000U);
    uart_printf("[TEST 1] got=%u value=%u (expect got=1 value=111)\r\n",
                (unsigned int)got, (unsigned int)value);
    uart_puts((got == 1U && value == 111U) ? "[TEST 1] PASS\r\n\r\n" : "[TEST 1] FAIL\r\n\r\n");

    uart_puts("[TEST 2] timeout with no notification\r\n");
    uint32_t before = systick_get_ms();
    got = task_notify_wait(&value, 300U);
    uint32_t elapsed = systick_get_ms() - before;
    uart_printf("[TEST 2] got=%u elapsed=%ums (expect got=0, elapsed~300ms)\r\n",
                (unsigned int)got, (unsigned int)elapsed);
    uart_puts((got == 0U) ? "[TEST 2] PASS\r\n\r\n" : "[TEST 2] FAIL\r\n\r\n");

    uart_puts("[TEST 3] wait-then-give: blocking now, another task will\r\n");
    uart_puts("         notify this one ~200ms later\r\n");
    before = systick_get_ms();
    got = task_notify_wait(&value, 2000U);
    elapsed = systick_get_ms() - before;
    uart_printf("[TEST 3] got=%u value=%u elapsed=%ums (expect got=1 value=222 elapsed~200ms)\r\n",
                (unsigned int)got, (unsigned int)value, (unsigned int)elapsed);
    uart_puts((got == 1U && value == 222U) ? "[TEST 3] PASS\r\n\r\n" : "[TEST 3] FAIL\r\n\r\n");

    uart_puts("[TEST 4] ISR-safe give: waiting on a notification that will\r\n");
    uart_puts("         arrive from a REAL SysTick interrupt, 500ms from now\r\n");
    s_isr_arm_countdown = 500U;   /* armed relative to NOW, not boot time */
    s_isr_notify_armed  = 1U;
    before = systick_get_ms();
    got = task_notify_wait(&value, 2000U);
    elapsed = systick_get_ms() - before;
    uart_printf("[TEST 4] got=%u value=0x%x elapsed=%ums (expect got=1 value=0xABCD, ~500ms)\r\n",
                (unsigned int)got, (unsigned int)value, (unsigned int)elapsed);
    uart_puts((got == 1U && value == 0xABCDU) ? "[TEST 4] PASS\r\n\r\n" : "[TEST 4] FAIL\r\n\r\n");

    uart_puts("=== Done ===\r\n");
    for (;;) { }
}

static void notifier_task(void)
{
    sched_delay_ms(1500U);
    if (s_waiter_handle != 0) {
        task_notify_give(s_waiter_handle, 222U);
    }

    for (;;) {
        sched_delay_ms(1000U);
    }
}

int main(void)
{
    mcg_init_120mhz();
    systick_init(120000000UL);
    uart_init(115200U);

    uart_puts("TamgaOS K64F — Task Notification Test\r\n\r\n");

    sched_init();
    sched_task_create(waiter_task, 1U);
    sched_task_create(notifier_task, 1U);
    sched_task_create(keepalive_task, 2U);
    sched_start();

    for (;;) { }
    return 0;
}