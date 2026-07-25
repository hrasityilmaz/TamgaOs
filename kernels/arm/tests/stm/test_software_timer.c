/*
 * tests/test_software_timer.c — validates one-shot and auto-reload
 * software timers.
 */

#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "timer.h"

static volatile uint32_t s_oneshot_fired  = 0U;
static volatile uint32_t s_periodic_count = 0U;

static void oneshot_callback(void *arg)
{
    (void)arg;
    s_oneshot_fired = 1U;
    uart_puts("[TIMER] one-shot fired\r\n");
}

static void periodic_callback(void *arg)
{
    (void)arg;
    s_periodic_count++;
    uart_printf("[TIMER] periodic fired, count=%u\r\n",
                (unsigned int)s_periodic_count);
}

static void test_basic_fire_counts(void)
{
    s_oneshot_fired  = 0U;
    s_periodic_count = 0U;
    uart_puts("\r\n[TEST 1] one-shot (500ms) + periodic (200ms)\r\n");
    timer_handle_t oneshot  = timer_create(500U, false, oneshot_callback, 0);
    timer_handle_t periodic = timer_create(200U, true,  periodic_callback, 0);
    if (oneshot < 0 || periodic < 0) {
        uart_puts("[TEST 1] FAIL: timer_create returned -1 (pool full?)\r\n");
        return;
    }

    timer_start(oneshot);
    timer_start(periodic);
    uint32_t start = systick_get_ms();
    while ((systick_get_ms() - start) < 2200U) {
        timer_service_tick();
    }

    timer_stop(periodic);
    timer_delete(oneshot);
    timer_delete(periodic);
    uart_printf("[TEST 1] one-shot fired=%u (expect 1)\r\n",
                (unsigned int)s_oneshot_fired);
    uart_printf("[TEST 1] periodic count=%u (expect ~10-12)\r\n",
                (unsigned int)s_periodic_count);
    if (s_oneshot_fired == 1U && s_periodic_count >= 10U && s_periodic_count <= 12U) {
        uart_puts("[TEST 1] PASS\r\n\r\n");
    } else {
        uart_puts("[TEST 1] FAIL\r\n\r\n");
    }
}

static void test_stop_halts_timer(void)
{
    s_periodic_count = 0U;
    uart_puts("[TEST 2] timer_stop() halts an active periodic timer\r\n");
    timer_handle_t t = timer_create(100U, true, periodic_callback, 0);
    if (t < 0) {
        uart_puts("[TEST 2] FAIL: timer_create returned -1\r\n");
        return;
    }
    timer_start(t);

    uint32_t start = systick_get_ms();
    while ((systick_get_ms() - start) < 550U) {
        timer_service_tick();
    }

    timer_stop(t);
    uint32_t count_at_stop = s_periodic_count;
    start = systick_get_ms();
    while ((systick_get_ms() - start) < 500U) {
        timer_service_tick();
    }

    timer_delete(t);
    uart_printf("[TEST 2] count at stop=%u, count after=%u (must match)\r\n",
                (unsigned int)count_at_stop, (unsigned int)s_periodic_count);
    if (count_at_stop == s_periodic_count && count_at_stop > 0U) {
        uart_puts("[TEST 2] PASS\r\n\r\n");
    } else {
        uart_puts("[TEST 2] FAIL\r\n\r\n");
    }
}

static void test_pool_exhaustion(void)
{
    uart_puts("[TEST 3] pool exhaustion (TIMER_MAX_COUNT limit)\r\n");
    timer_handle_t handles[TIMER_MAX_COUNT];
    uint8_t created = 0U;
    for (uint8_t i = 0U; i < TIMER_MAX_COUNT; i++) {
        handles[i] = timer_create(1000U, false, oneshot_callback, 0);
        if (handles[i] >= 0) {
            created++;
        }
    }

    timer_handle_t overflow = timer_create(1000U, false, oneshot_callback, 0);
    uart_printf("[TEST 3] created=%u (expect %u), overflow handle=%d (expect -1)\r\n",
                (unsigned int)created, (unsigned int)TIMER_MAX_COUNT, (int)overflow);
    for (uint8_t i = 0U; i < TIMER_MAX_COUNT; i++) {
        if (handles[i] >= 0) {
            timer_delete(handles[i]);
        }
    }
    if (created == TIMER_MAX_COUNT && overflow == -1) {
        uart_puts("[TEST 3] PASS\r\n\r\n");
    } else {
        uart_puts("[TEST 3] FAIL\r\n\r\n");
    }
}

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();
    uart_puts("TamgaOS STM32H753ZI @ 480MHz\r\n");
    uart_puts("=== Software Timer Test Suite ===\r\n");
    test_basic_fire_counts();
    test_stop_halts_timer();
    test_pool_exhaustion();
    uart_puts("=== Done ===\r\n");
    for (;;) { }
    return 0;
}