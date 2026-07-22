/*
 * test_timeout.c
 *
 * Purpose: Prove that mutex_lock_timeout / queue_send_timeout /
 *          queue_receive_timeout correctly return -1 when the
 *          timeout expires, and 0 when the resource becomes
 *          available before the deadline — instead of blocking
 *          forever like the plain (non-timeout) variants.
 *
 * ------------------------------------------------------------------
 * TEST 1 — mutex_lock_timeout EXPIRES (nobody ever releases)
 * ------------------------------------------------------------------
 *   task_mutex_holder grabs g_mutex_never_released and never lets go.
 *   task_mutex_waiter tries mutex_lock_timeout(300ms) on it.
 *
 *   Expected: after ~300ms, mutex_lock_timeout returns -1, and the
 *             waiting task is NOT stuck forever.
 *
 *   Expected log:
 *     [HOLDER] grabbed mutex, holding forever
 *     [WAITER] trying mutex_lock_timeout(300ms)...
 *     [WAITER] TIMED OUT as expected (result=-1)
 *
 * ------------------------------------------------------------------
 * TEST 2 — mutex_lock_timeout SUCCEEDS (released just in time)
 * ------------------------------------------------------------------
 *   task_mutex_releaser grabs g_mutex_released_late, holds it for
 *   150ms, then releases it.
 *   task_mutex_waiter2 tries mutex_lock_timeout(500ms) on it —
 *   500ms > 150ms, so it should succeed well before the deadline.
 *
 *   Expected log:
 *     [REL]    grabbed mutex, will release in 150ms
 *     [WAIT2]  trying mutex_lock_timeout(500ms)...
 *     [REL]    releasing mutex now
 *     [WAIT2]  got the mutex before timeout (result=0)
 *
 * ------------------------------------------------------------------
 * TEST 3 — queue_receive_timeout EXPIRES (nobody ever sends)
 * ------------------------------------------------------------------
 *   task_queue_waiter calls queue_receive_timeout(300ms) on an
 *   always-empty queue. Nobody ever sends to it.
 *
 *   Expected log:
 *     [QWAIT]  trying queue_receive_timeout(300ms) on empty queue...
 *     [QWAIT]  TIMED OUT as expected (result=-1)
 *
 * ------------------------------------------------------------------
 * TEST 4 — queue_receive_timeout SUCCEEDS (data arrives in time)
 * ------------------------------------------------------------------
 *   task_queue_sender waits 150ms, then sends one item.
 *   task_queue_waiter2 calls queue_receive_timeout(500ms) — should
 *   receive the item well before the deadline.
 *
 *   Expected log:
 *     [QWAIT2] trying queue_receive_timeout(500ms)...
 *     [QSEND]  sending item now (after 150ms)
 *     [QWAIT2] got item before timeout (result=0, value=77)
 *
 * ------------------------------------------------------------------
 * TEST 5 — queue_send_timeout EXPIRES (queue full, nobody drains it)
 * ------------------------------------------------------------------
 *   g_q_full_test has capacity=1 and starts pre-filled.
 *   task_queue_send_waiter tries queue_send_timeout(300ms) on it —
 *   queue stays full the whole time, nobody ever receives.
 *
 *   Expected log:
 *     [QSWAIT] trying queue_send_timeout(300ms) on full queue...
 *     [QSWAIT] TIMED OUT as expected (result=-1)
 *
 * All five tests run in a single boot, staggered by initial
 * sched_delay_ms() offsets so their logs interleave predictably
 * without needing separate boots.
 */

#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "scheduler.h"
#include "task.h"
#include "mutex.h"
#include "queue.h"
#include "fault_log.h"

static mutex_t g_mutex_never_released;
static mutex_t g_mutex_released_late;
QUEUE_DEFINE(g_q_recv_timeout_test, uint32_t, 4U);
QUEUE_DEFINE(g_q_full_test, uint32_t, 1U);
static void task_mutex_holder(void)
{
    sched_delay_ms(50U);
    uart_puts("[HOLDER] grabbed mutex, holding forever\r\n");
    mutex_lock(&g_mutex_never_released);
    for (;;) {
        sched_delay_ms(10000U);
    }
}

static void task_mutex_waiter(void)
{
    sched_delay_ms(100U);   /* let HOLDER grab it first */
    uart_puts("[WAITER] trying mutex_lock_timeout(300ms)...\r\n");
    int result = mutex_lock_timeout(&g_mutex_never_released, 300U);
    if (result == 0) {
        uart_puts("[WAITER] UNEXPECTED: got the mutex (should have timed out!)\r\n");
        mutex_unlock(&g_mutex_never_released);
    } else {
        uart_puts("[WAITER] TIMED OUT as expected (result=-1)\r\n");
    }
    for (;;) {
        sched_delay_ms(10000U);
    }
}

static void task_mutex_releaser(void)
{
    sched_delay_ms(50U);
    uart_puts("[REL]    grabbed mutex, will release in 150ms\r\n");
    mutex_lock(&g_mutex_released_late);
    sched_delay_ms(150U);
    uart_puts("[REL]    releasing mutex now\r\n");
    mutex_unlock(&g_mutex_released_late);

    for (;;) {
        sched_delay_ms(10000U);
    }
}

static void task_mutex_waiter2(void)
{
    sched_delay_ms(100U); 
    uart_puts("[WAIT2]  trying mutex_lock_timeout(500ms)...\r\n");
    int result = mutex_lock_timeout(&g_mutex_released_late, 500U);

    if (result == 0) {
        uart_puts("[WAIT2]  got the mutex before timeout (result=0)\r\n");
        mutex_unlock(&g_mutex_released_late);
    } else {
        uart_puts("[WAIT2]  UNEXPECTED: timed out (should have succeeded!)\r\n");
    }

    for (;;) {
        sched_delay_ms(10000U);
    }
}

static void task_queue_waiter(void)
{
    sched_delay_ms(50U);

    uint32_t val;
    uart_puts("[QWAIT]  trying queue_receive_timeout(300ms) on empty queue...\r\n");
    int result = queue_receive_timeout(&g_q_recv_timeout_test, &val, 300U);

    if (result == 0) {
        uart_printf("[QWAIT]  UNEXPECTED: got value %d (should have timed out!)\r\n", (int)val);
    } else {
        uart_puts("[QWAIT]  TIMED OUT as expected (result=-1)\r\n");
    }

    for (;;) {
        sched_delay_ms(10000U);
    }
}

static void task_queue_waiter2(void)
{
    sched_delay_ms(400U);   /* start after test 3 has already timed out */

    uint32_t val;
    uart_puts("[QWAIT2] trying queue_receive_timeout(500ms)...\r\n");
    int result = queue_receive_timeout(&g_q_recv_timeout_test, &val, 500U);

    if (result == 0) {
        uart_printf("[QWAIT2] got item before timeout (result=0, value=%d)\r\n", (int)val);
    } else {
        uart_puts("[QWAIT2] UNEXPECTED: timed out (should have succeeded!)\r\n");
    }

    for (;;) {
        sched_delay_ms(10000U);
    }
}

static void task_queue_sender(void)
{
    sched_delay_ms(450U);   /* wait a bit after QWAIT2 starts blocking */
    uart_puts("[QSEND]  sending item now (after 150ms wait from QWAIT2's perspective)\r\n");
    uint32_t v = 77U;
    queue_send(&g_q_recv_timeout_test, &v);

    for (;;) {
        sched_delay_ms(10000U);
    }
}

static void task_queue_send_waiter(void)
{
    sched_delay_ms(900U);   /* run after earlier tests are done, own timeslot */

    uart_puts("[QSWAIT] trying queue_send_timeout(300ms) on full queue...\r\n");
    uint32_t v = 55U;
    int result = queue_send_timeout(&g_q_full_test, &v, 300U);

    if (result == 0) {
        uart_puts("[QSWAIT] UNEXPECTED: send succeeded (queue should have stayed full!)\r\n");
    } else {
        uart_puts("[QSWAIT] TIMED OUT as expected (result=-1)\r\n");
    }
    uart_puts("=== ALL TIMEOUT TESTS DONE ===\r\n");
    for (;;) {
        sched_delay_ms(10000U);
    }
}

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();

    fault_log_init();
    fault_log_t f;
    if (fault_log_check_and_clear(&f)) {
        uart_puts("=== Previous boot ended in a fault ===\r\n");
        uart_printf("PC=0x%x LR=0x%x xPSR=0x%x\r\n", f.pc, f.lr, f.xpsr);
        uart_printf("CFSR=0x%x HFSR=0x%x\r\n", f.cfsr, f.hfsr);
        if (f.mmfar_valid) uart_printf("MMFAR=0x%x\r\n", f.mmfar);
        if (f.bfar_valid)  uart_printf("BFAR=0x%x\r\n", f.bfar);
    }

    uart_puts("TamgaOS STM32H753ZI @ 480MHz\r\n");
    uart_puts("Mutex/Queue Timeout Test\r\n\r\n");

    mutex_init(&g_mutex_never_released);
    mutex_init(&g_mutex_released_late);

    queue_init(&g_q_recv_timeout_test, g_q_recv_timeout_test_storage,
               sizeof(uint32_t), 4U);

    queue_init(&g_q_full_test, g_q_full_test_storage, sizeof(uint32_t), 1U);
    {
        uint32_t filler = 999U;
        queue_try_send(&g_q_full_test, &filler);
    }

    sched_init();
    sched_task_create(task_mutex_holder,      TASK_PRIORITY_LOW);
    sched_task_create(task_mutex_waiter,      TASK_PRIORITY_HIGH);
    sched_task_create(task_mutex_releaser,    TASK_PRIORITY_LOW);
    sched_task_create(task_mutex_waiter2,     TASK_PRIORITY_HIGH);
    sched_task_create(task_queue_waiter,      TASK_PRIORITY_NORMAL);
    sched_task_create(task_queue_waiter2,     TASK_PRIORITY_NORMAL);
    sched_task_create(task_queue_sender,      TASK_PRIORITY_LOW);
    sched_task_create(task_queue_send_waiter, TASK_PRIORITY_NORMAL);
    sched_start();

    return 0;
}