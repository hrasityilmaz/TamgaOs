/*
 * test_event_flags.c (K64F port)
 *
 * Purpose: Prove that event_group_t works correctly on K64F — same
 * five scenarios as the STM32 version (ANY-wake, ALL-wake, auto_clear,
 * timeout-expiry, timeout-success). See tests/test_event_flags.c for
 * full phase-by-phase documentation.
 *
 * NOTE: This test creates 10 tasks total — requires TASK_MAX >= 10 in
 * task.h (already bumped to 12 for the STM32 port's event flags test;
 * task.h is shared, so this should already be satisfied).
 */

#include "mcg.h"
#include "systick.h"
#include "uart.h"
#include "scheduler.h"
#include "task.h"
#include "event.h"

/* ---- Local print helper ---- */
static void uart_put_hex32(uint32_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) {
        uart_putc(hex[(v >> i) & 0xFU]);
    }
}

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
    uart_puts("[ANY-WAIT]  woke up, observed_bits=0x");
    uart_put_hex32(observed);
    uart_puts("\r\n");

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
    uart_puts("[ALL-WAIT]  woke up, observed_bits=0x");
    uart_put_hex32(observed);
    uart_puts("\r\n");

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
                                    EVENT_WAIT_ANY, 1);
    uart_puts("[CLR-WAIT]  woke up, observed_bits=0x");
    uart_put_hex32(observed);
    uart_puts("\r\n");

    for (;;) {
        sched_delay_ms(10000U);
    }
}

static void task_clear_setter(void)
{
    sched_delay_ms(500U);
    uart_puts("[CLR-SET]   setting BIT_E\r\n");
    event_set(&g_evt_clear_test, BIT_E);

    sched_delay_ms(50U);
    uint32_t remaining = event_get(&g_evt_clear_test);
    uart_puts("[CLR-SET]   post-wake check: event_get()=0x");
    uart_put_hex32(remaining);
    uart_puts("\r\n");

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
        uart_puts("[TO-WAIT]   UNEXPECTED: got bits 0x");
        uart_put_hex32(result);
        uart_puts(" (should have timed out!)\r\n");
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
        uart_puts("[TO2-WAIT]  got it before timeout, observed_bits=0x");
        uart_put_hex32(result);
        uart_puts("\r\n");
    }

    uart_puts("=== ALL EVENT FLAG TESTS DONE ===\r\n");

    for (;;) {
        sched_delay_ms(10000U);
    }
}

static void task_timeout_setter(void)
{
    sched_delay_ms(1200U);
    uart_puts("[TO2-SET]   setting BIT_G (after 150ms)\r\n");
    event_set(&g_evt_timeout_test2, BIT_G);

    for (;;) {
        sched_delay_ms(10000U);
    }
}

int main(void)
{
    mcg_init_120mhz();
    uart_init(115200U);

    uart_puts("TamgaOS K64F @ 120MHz\r\n");
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

    systick_init(120000000UL);
    systick_sched_enable();

    sched_start();

    return 0;
}