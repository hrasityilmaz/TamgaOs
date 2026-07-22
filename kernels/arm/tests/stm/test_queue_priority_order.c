/*
 * test_queue_priority_order.c
 *
 * TODO: MUST CONTROL !!!
 * Sometimes logic coming weird but should be correct 
 * When ı am not sleepy ı will check again :)
 * 
 * 
 * NOTE (checked after 16/07/2026): The print order in the log may
 * appear NOT to match the source code order one-to-one (for example,
 * the "[SMED] send done!" line shows up IN THE MIDDLE of LOW's
 * queue_receive() call). This is NOT A BUG — sched_wake_task() triggers
 * PendSV IMMEDIATELY when it wakes a higher-priority task, preempting
 * before LOW's function even returns. This is INTENTIONAL and CORRECT
 * RTOS behavior (a higher-priority task should start running as soon
 * as possible once it becomes ready).
 * What actually matters is that the VALUES arrive in the correct order
 * (20 first, then 10) — this was IDENTICAL across two separate
 * captures, meaning it's fully deterministic. The print order is
 * confusing, but the test PASSES.
 *
 *
 * Purpose: Prove that the priority-ordered wait list (wq_insert/wq_pop)
 *          in queue.c works correctly for both the receive side and
 *          the send side. A waiter joining the wait list EARLIER does
 *          NOT make it get served first — only its PRIORITY value
 *          should determine wake order (priority-order, not FIFO).
 *
 *
 */

#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "scheduler.h"
#include "task.h"
#include "queue.h"

QUEUE_DEFINE(g_q_recv_test, uint32_t, 4U);
QUEUE_DEFINE(g_q_send_test, uint32_t, 1U);
static volatile uint8_t s_phase2_recv_count;

static void task_recv_med(void)
{
    sched_delay_ms(50U); 

    uint32_t val;
    for (;;) {
        uart_puts("[MED]  receive  (empty, blocked)\r\n");
        queue_receive(&g_q_recv_test, &val);
        uart_printf("[MED]  get value: %d\r\n", (int)val);
        sched_delay_ms(5000U); 
    }
}

static void task_recv_high(void)
{
    sched_delay_ms(150U); 
    uint32_t val;
    for (;;) {
        uart_puts("[HIGH] receive request (empty, blocked)\r\n");
        queue_receive(&g_q_recv_test, &val);
        uart_printf("[HIGH] get: %d\r\n", (int)val);
        sched_delay_ms(5000U);
    }
}

static void task_send_low_phase1(void)
{
    sched_delay_ms(300U);
    uart_puts("[LOW]  sending: 100\r\n");
    queue_send(&g_q_recv_test, &(uint32_t){100U});
    sched_delay_ms(200U); 
    uart_puts("[LOW]  sending: 200\r\n");
    queue_send(&g_q_recv_test, &(uint32_t){200U});

    for (;;) {
        sched_delay_ms(10000U);
    }
}

static void task_send_med_phase2(void)
{
    sched_delay_ms(600U); 

    uart_puts("[SMED] send request: 10 (full, blocked)\r\n");
    queue_send(&g_q_send_test, &(uint32_t){10U});
    uart_puts("[SMED] send done!\r\n");

    for (;;) {
        sched_delay_ms(10000U);
    }
}

static void task_send_high_phase2(void)
{
    sched_delay_ms(700U); 
    uart_puts("[SHI]  send :: 20 (full, blocked)\r\n");
    queue_send(&g_q_send_test, &(uint32_t){20U});
    uart_puts("[SHI]  sending done...\r\n");
    for (;;) {
        sched_delay_ms(10000U);
    }
}

static void task_recv_low_phase2(void)
{
    sched_delay_ms(900U); 
    uint32_t val;

    uart_puts("[LOW]  slot opening..\r\n");
    queue_receive(&g_q_send_test, &val);
    uart_printf("[LOW]  First value: %d (starting point)\r\n", (int)val);
    s_phase2_recv_count++;
    sched_delay_ms(100U); 
    uart_puts("[LOW]  second item.\r\n");
    queue_receive(&g_q_send_test, &val);
    uart_printf("[LOW]  second item value: %d  <-- must be 20 (SHI, HIGH priority)\r\n", (int)val);
    s_phase2_recv_count++;
    sched_delay_ms(100U);
    uart_puts("[LOW]  third item.\r\n");
    queue_receive(&g_q_send_test, &val);
    uart_printf("[LOW]  Third item value: %d  <-- Must be 10 (SMED, NORMAL priority)\r\n", (int)val);
    s_phase2_recv_count++;
    uart_puts("=== TEST DONE ===\r\n");

    for (;;) {
        sched_delay_ms(10000U);
    }
}

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();

    uart_puts("TamgaOS STM32H753ZI @ 480MHz\r\n");
    uart_puts("Queue Priority-Ordered Wait Test\r\n");
    uart_puts("Faz 1: receive-side priority (queue bos)\r\n");
    uart_puts("Faz 2: send-side priority (queue dolu)\r\n\r\n");

    queue_init(&g_q_recv_test, g_q_recv_test_storage, sizeof(uint32_t), 4U);

    queue_init(&g_q_send_test, g_q_send_test_storage, sizeof(uint32_t), 1U);
    queue_send(&g_q_send_test, &(uint32_t){999U}); 

    sched_init();
    sched_task_create(task_recv_med,        TASK_PRIORITY_NORMAL);
    sched_task_create(task_recv_high,       TASK_PRIORITY_HIGH);
    sched_task_create(task_send_low_phase1, TASK_PRIORITY_LOW);
    sched_task_create(task_send_med_phase2, TASK_PRIORITY_NORMAL);
    sched_task_create(task_send_high_phase2,TASK_PRIORITY_HIGH);
    sched_task_create(task_recv_low_phase2, TASK_PRIORITY_LOW);
    sched_start();

    return 0;
}