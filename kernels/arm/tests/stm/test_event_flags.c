/*
 * test_event_flags.c
 *
 * Purpose: Prove that event_group_t works correctly for:
 *   - EVENT_WAIT_ANY (wakes as soon as ONE of the requested bits is set)
 *   - EVENT_WAIT_ALL (wakes only once ALL requested bits are set)
 *   - auto_clear behavior (matched bits are cleared atomically on wake)
 *   - event_wait_timeout expiring when the condition is never met
 *   - event_wait_timeout succeeding before the deadline
 *
 * ------------------------------------------------------------------
 * TEST 1 — EVENT_WAIT_ANY wakes on the FIRST bit set, not the last
 * ------------------------------------------------------------------
 *   task_any_waiter waits on (BIT_A | BIT_B) with EVENT_WAIT_ANY.
 *   task_any_setter sets BIT_A only, after 100ms. BIT_B is never set.
 *
 *   Expected: task_any_waiter wakes as soon as BIT_A is set — it must
 *             NOT wait for BIT_B, since ANY-mode only needs one.
 *
 *   Expected log:
 *     [ANY-WAIT]  waiting for BIT_A|BIT_B (mode=ANY)...
 *     [ANY-SET]   setting BIT_A only
 *     [ANY-WAIT]  woke up, observed_bits=0x1  <-- only BIT_A, as expected
 *
 * ------------------------------------------------------------------
 * TEST 2 — EVENT_WAIT_ALL waits for BOTH bits, not just the first
 * ------------------------------------------------------------------
 *   task_all_waiter waits on (BIT_C | BIT_D) with EVENT_WAIT_ALL.
 *   task_all_setter1 sets BIT_C after 100ms.
 *   task_all_setter2 sets BIT_D after 300ms.
 *
 *   Expected: task_all_waiter does NOT wake after BIT_C alone (at
 *             100ms) — it must keep waiting until BIT_D also arrives
 *             at 300ms.
 *
 *   Expected log:
 *     [ALL-WAIT]  waiting for BIT_C&BIT_D (mode=ALL)...
 *     [ALL-SET1]  setting BIT_C only (BIT_D not set yet)
 *     [ALL-SET2]  setting BIT_D (both should be set now)
 *     [ALL-WAIT]  woke up, observed_bits=0xC  <-- both bits, and only
 *                                                  after BIT_D arrived
 *
 * ------------------------------------------------------------------
 * TEST 3 — auto_clear removes the matched bits before the next check
 * ------------------------------------------------------------------
 *   task_clear_setter sets BIT_E, then immediately checks event_get()
 *   — since task_clear_waiter consumed it with auto_clear=1, BIT_E
 *   should already be gone by the time we check.
 *
 *   Expected log:
 *     [CLR-WAIT]  waiting for BIT_E (auto_clear=1)...
 *     [CLR-SET]   setting BIT_E
 *     [CLR-WAIT]  woke up, observed_bits=0x10
 *     [CLR-SET]   post-wake check: event_get()=0x0  <-- bit was cleared
 *
 * ------------------------------------------------------------------
 * TEST 4 — event_wait_timeout EXPIRES (bit never set)
 * ------------------------------------------------------------------
 *   task_timeout_waiter waits on BIT_F with a 300ms timeout. Nobody
 *   ever sets BIT_F.
 *
 *   Expected log:
 *     [TO-WAIT]   waiting for BIT_F with 300ms timeout...
 *     [TO-WAIT]   TIMED OUT as expected (result=0xFFFFFFFF)
 *
 * ------------------------------------------------------------------
 * TEST 5 — event_wait_timeout SUCCEEDS before the deadline
 * ------------------------------------------------------------------
 *   task_timeout_waiter2 waits on BIT_G with a 500ms timeout.
 *   task_timeout_setter sets BIT_G after 150ms — well within budget.
 *
 *   Expected log:
 *     [TO2-WAIT]  waiting for BIT_G with 500ms timeout...
 *     [TO2-SET]   setting BIT_G (after 150ms)
 *     [TO2-WAIT]  got it before timeout, observed_bits=0x40
 */

#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "scheduler.h"
#include "task.h"
#include "event.h"

#define BIT_A (1U << 0U)
#define BIT_B (1U << 1U)
#define BIT_C (1U << 2U)
#define BIT_D (1U << 3U)
#define BIT_E (1U << 4U)
#define BIT_F (1U << 5U)
#define BIT_G (1U << 6U)

static event_group_t g_evt_any_test;
static event_group_t g_evt_all_test;
static event_group_t g_evt_clear_test;
static event_group_t g_evt_timeout_test1;
static event_group_t g_evt_timeout_test2;

/* ================= TEST 1: EVENT_WAIT_ANY ================= */

static void task_any_waiter(void)
{
    sched_delay_ms(50U);

    uart_puts("[ANY-WAIT]  waiting for BIT_A|BIT_B (mode=ANY)...\r\n");
    uint32_t observed = event_wait(&g_evt_any_test, BIT_A | BIT_B,
                                    EVENT_WAIT_ANY, 0);
    uart_printf("[ANY-WAIT]  woke up, observed_bits=0x%x\r\n", (unsigned int)observed);

    for (;;) {
        sched_delay_ms(10000U);
    }
}

static void task_any_setter(void)
{
    sched_delay_ms(100U);
    uart_puts("[ANY-SET]   setting BIT_A only\r\n");
    event_set(&g_evt_any_test, BIT_A);

    for (;;) {
        sched_delay_ms(10000U);
    }
}

/* ================= TEST 2: EVENT_WAIT_ALL ================= */

static void task_all_waiter(void)
{
    sched_delay_ms(150U);

    uart_puts("[ALL-WAIT]  waiting for BIT_C&BIT_D (mode=ALL)...\r\n");
    uint32_t observed = event_wait(&g_evt_all_test, BIT_C | BIT_D,
                                    EVENT_WAIT_ALL, 0);
    uart_printf("[ALL-WAIT]  woke up, observed_bits=0x%x\r\n", (unsigned int)observed);

    for (;;) {
        sched_delay_ms(10000U);
    }
}

static void task_all_setter1(void)
{
    sched_delay_ms(200U);
    uart_puts("[ALL-SET1]  setting BIT_C only (BIT_D not set yet)\r\n");
    event_set(&g_evt_all_test, BIT_C);

    for (;;) {
        sched_delay_ms(10000U);
    }
}

static void task_all_setter2(void)
{
    sched_delay_ms(400U);
    uart_puts("[ALL-SET2]  setting BIT_D (both should be set now)\r\n");
    event_set(&g_evt_all_test, BIT_D);

    for (;;) {
        sched_delay_ms(10000U);
    }
}

/* ================= TEST 3: auto_clear ================= */

static void task_clear_waiter(void)
{
    sched_delay_ms(450U);

    uart_puts("[CLR-WAIT]  waiting for BIT_E (auto_clear=1)...\r\n");
    uint32_t observed = event_wait(&g_evt_clear_test, BIT_E,
                                    EVENT_WAIT_ANY, 1 /* auto_clear */);
    uart_printf("[CLR-WAIT]  woke up, observed_bits=0x%x\r\n", (unsigned int)observed);

    for (;;) {
        sched_delay_ms(10000U);
    }
}

static void task_clear_setter(void)
{
    sched_delay_ms(500U);
    uart_puts("[CLR-SET]   setting BIT_E\r\n");
    event_set(&g_evt_clear_test, BIT_E);

    sched_delay_ms(50U);   /* give the waiter time to wake and clear it */
    uint32_t remaining = event_get(&g_evt_clear_test);
    uart_printf("[CLR-SET]   post-wake check: event_get()=0x%x\r\n",
                (unsigned int)remaining);

    for (;;) {
        sched_delay_ms(10000U);
    }
}

/* ================= TEST 4: timeout EXPIRES ================= */

static void task_timeout_waiter(void)
{
    sched_delay_ms(650U);

    uart_puts("[TO-WAIT]   waiting for BIT_F with 300ms timeout...\r\n");
    uint32_t result = event_wait_timeout(&g_evt_timeout_test1, BIT_F,
                                          EVENT_WAIT_ANY, 0, 300U);

    if (result == 0xFFFFFFFFU) {
        uart_puts("[TO-WAIT]   TIMED OUT as expected (result=0xFFFFFFFF)\r\n");
    } else {
        uart_printf("[TO-WAIT]   UNEXPECTED: got bits 0x%x (should have timed out!)\r\n",
                    (unsigned int)result);
    }

    for (;;) {
        sched_delay_ms(10000U);
    }
}

/* ================= TEST 5: timeout SUCCEEDS ================= */

static void task_timeout_waiter2(void)
{
    sched_delay_ms(1050U);

    uart_puts("[TO2-WAIT]  waiting for BIT_G with 500ms timeout...\r\n");
    uint32_t result = event_wait_timeout(&g_evt_timeout_test2, BIT_G,
                                          EVENT_WAIT_ANY, 0, 500U);

    if (result == 0xFFFFFFFFU) {
        uart_puts("[TO2-WAIT]  UNEXPECTED: timed out (should have succeeded!)\r\n");
    } else {
        uart_printf("[TO2-WAIT]  got it before timeout, observed_bits=0x%x\r\n",
                    (unsigned int)result);
    }

    uart_puts("=== ALL EVENT FLAG TESTS DONE ===\r\n");

    for (;;) {
        sched_delay_ms(10000U);
    }
}

static void task_timeout_setter(void)
{
    sched_delay_ms(1200U);   /* 150ms after task_timeout_waiter2 starts waiting */
    uart_puts("[TO2-SET]   setting BIT_G (after 150ms)\r\n");
    event_set(&g_evt_timeout_test2, BIT_G);

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
    uart_puts("Event Flags Test\r\n\r\n");

    event_init(&g_evt_any_test);
    event_init(&g_evt_all_test);
    event_init(&g_evt_clear_test);
    event_init(&g_evt_timeout_test1);
    event_init(&g_evt_timeout_test2);

    sched_init();
    sched_task_create(task_any_waiter,       TASK_PRIORITY_NORMAL);
    sched_task_create(task_any_setter,       TASK_PRIORITY_LOW);
    sched_task_create(task_all_waiter,       TASK_PRIORITY_NORMAL);
    sched_task_create(task_all_setter1,      TASK_PRIORITY_LOW);
    sched_task_create(task_all_setter2,      TASK_PRIORITY_LOW);
    sched_task_create(task_clear_waiter,     TASK_PRIORITY_NORMAL);
    sched_task_create(task_clear_setter,     TASK_PRIORITY_LOW);
    sched_task_create(task_timeout_waiter,   TASK_PRIORITY_NORMAL);
    sched_task_create(task_timeout_waiter2,  TASK_PRIORITY_NORMAL);
    sched_task_create(task_timeout_setter,   TASK_PRIORITY_LOW);
    sched_start();

    return 0;
}