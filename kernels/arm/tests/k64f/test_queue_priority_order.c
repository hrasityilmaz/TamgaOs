/*
 * test_queue_priority_order.c (K64F port)
 *
 * Purpose: Prove that the priority-ordered wait list (wq_insert/wq_pop)
 *          in queue.c works correctly for both the receive side and
 *          the send side, on K64F — same logic as the STM32 port,
 *          verifying that kernel/core/queue.c is genuinely
 *          board-agnostic.
 *
 * See tests/test_queue_priority_order.c (STM32 version) for full
 * phase-by-phase documentation — the test structure here is identical,
 * only the printing mechanism differs (no uart_printf on this board).
 */

#include "mcg.h"
#include "systick.h"
#include "uart.h"
#include "scheduler.h"
#include "task.h"
#include "queue.h"

/* ---- Local print helpers — this board's uart.c has no printf ---- */

static void uart_put_dec(uint32_t v)
{
    char buf[10];
    int i = 0;

    if (v == 0U) {
        uart_putc('0');
        return;
    }
    while (v > 0U) {
        buf[i++] = (char)('0' + (v % 10U));
        v /= 10U;
    }
    while (i > 0) {
        uart_putc(buf[--i]);
    }
}

/* ---- Phase 1 queue (receive-side test) ---- */
QUEUE_DEFINE(g_q_recv_test, uint32_t, 4U);

/* ---- Phase 2 queue (send-side test, capacity=1, pre-filled) ---- */
QUEUE_DEFINE(g_q_send_test, uint32_t, 1U);

/* ================= PHASE 1 TASKS ================= */

static void task_recv_med(void)
{
    sched_delay_ms(50U);

    uint32_t val;
    for (;;) {
        uart_puts("[MED]  requesting receive (empty, blocked)\r\n");
        queue_receive(&g_q_recv_test, &val);
        uart_puts("[MED]  received: ");
        uart_put_dec(val);
        uart_puts("\r\n");
        sched_delay_ms(5000U);
    }
}

static void task_recv_high(void)
{
    sched_delay_ms(150U);

    uint32_t val;
    for (;;) {
        uart_puts("[HIGH] requesting receive (empty, blocked)\r\n");
        queue_receive(&g_q_recv_test, &val);
        uart_puts("[HIGH] received: ");
        uart_put_dec(val);
        uart_puts("\r\n");
        sched_delay_ms(5000U);
    }
}

static void task_send_low_phase1(void)
{
    sched_delay_ms(300U);

    uart_puts("[LOW]  sending: 100\r\n");
    {
        uint32_t v = 100U;
        queue_send(&g_q_recv_test, &v);
    }

    sched_delay_ms(200U);

    uart_puts("[LOW]  sending: 200\r\n");
    {
        uint32_t v = 200U;
        queue_send(&g_q_recv_test, &v);
    }

    for (;;) {
        sched_delay_ms(10000U);
    }
}

/* ================= PHASE 2 TASKS ================= */

static void task_send_med_phase2(void)
{
    sched_delay_ms(600U);

    uart_puts("[SMED] send requesting: 10 (full, blocked)\r\n");
    {
        uint32_t v = 10U;
        queue_send(&g_q_send_test, &v);
    }
    uart_puts("[SMED] send completed\r\n");

    for (;;) {
        sched_delay_ms(10000U);
    }
}

static void task_send_high_phase2(void)
{
    sched_delay_ms(700U);

    uart_puts("[SHI]  send requesting: 20 (full, blocked)\r\n");
    {
        uint32_t v = 20U;
        queue_send(&g_q_send_test, &v);
    }
    uart_puts("[SHI]  send completed\r\n");

    for (;;) {
        sched_delay_ms(10000U);
    }
}

static void task_recv_low_phase2(void)
{
    sched_delay_ms(900U);

    uint32_t val;

    uart_puts("[LOW]  taking first item (freeing a slot)\r\n");
    queue_receive(&g_q_send_test, &val);
    uart_puts("[LOW]  first item value: ");
    uart_put_dec(val);
    uart_puts(" (initial filler)\r\n");

    sched_delay_ms(100U);

    uart_puts("[LOW]  taking second item\r\n");
    queue_receive(&g_q_send_test, &val);
    uart_puts("[LOW]  second item value: ");
    uart_put_dec(val);
    uart_puts(" (expect 20 -- SHI, HIGH priority)\r\n");

    sched_delay_ms(100U);

    uart_puts("[LOW]  taking third item\r\n");
    queue_receive(&g_q_send_test, &val);
    uart_puts("[LOW]  third item value: ");
    uart_put_dec(val);
    uart_puts(" (expect 10 -- SMED, NORMAL priority)\r\n");

    uart_puts("=== TEST DONE ===\r\n");

    for (;;) {
        sched_delay_ms(10000U);
    }
}

int main(void)
{
    mcg_init_120mhz();
    uart_init(115200U);

    uart_puts("TamgaOS K64F @ 120MHz\r\n");
    uart_puts("Queue Priority-Ordered Wait Test\r\n\r\n");

    queue_init(&g_q_recv_test, g_q_recv_test_storage, sizeof(uint32_t), 4U);

    queue_init(&g_q_send_test, g_q_send_test_storage, sizeof(uint32_t), 1U);
    {
        uint32_t filler = 999U;
        queue_try_send(&g_q_send_test, &filler);
    }

    sched_init();
    sched_task_create(task_recv_med,        TASK_PRIORITY_NORMAL);
    sched_task_create(task_recv_high,       TASK_PRIORITY_HIGH);
    sched_task_create(task_send_low_phase1, TASK_PRIORITY_LOW);
    sched_task_create(task_send_med_phase2, TASK_PRIORITY_NORMAL);
    sched_task_create(task_send_high_phase2,TASK_PRIORITY_HIGH);
    sched_task_create(task_recv_low_phase2, TASK_PRIORITY_LOW);

    systick_init(120000000UL);
    systick_sched_enable();

    sched_start();

    return 0;
}