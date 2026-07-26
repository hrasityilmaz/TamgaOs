/*
 * tests/k64f/test_isr_safe_primitives.c 
 *
 * Scenario:
 *   - Every real 1ms SysTick tick, the ISR hook increments a
 *     counter. Every 500 ticks it pushes that counter into
 *     isr_queue and sets a bit in isr_event via the *_from_isr()
 *     variants.
 *   - A normal task blocks on queue_receive() (the ordinary,
 *     blocking version) and event_wait(), and reports how long it
 *     took to wake up after the ISR signaled it — this proves the
 *     ISR-safe wake path (sched_wake_task() called without going
 *     through the blocking for(;;) loop) actually unblocks a
 *     waiting task correctly.
 *   - A second phase deliberately floods queue_send_from_isr()
 *     faster than the queue can drain, to prove it returns -1
 *     immediately instead of hanging the ISR.
 *
 */

#include "mcg.h"
#include "systick.h"
#include "uart.h"
#include "scheduler.h"
#include "queue.h"
#include "event.h"

#define ISR_EVENT_BIT   (1UL << 0U)

static queue_t      s_isr_queue;
static uint32_t      s_isr_queue_buf[4];
static event_group_t s_isr_event;

static volatile uint32_t s_isr_tick_count  = 0U;
static volatile uint32_t s_isr_send_ok     = 0U;
static volatile uint32_t s_isr_send_failed = 0U;
static volatile uint8_t  s_flood_mode      = 0U; 

/*
 * This OVERRIDES the weak systick_isr_hook() — runs inside the real
 * SysTick interrupt, every tick.
 */
void systick_isr_hook(void)
{
    s_isr_tick_count++;

    if (s_flood_mode) {
        uint32_t val = s_isr_tick_count;
        if (queue_send_from_isr(&s_isr_queue, &val) == 0) {
            s_isr_send_ok++;
        } else {
            s_isr_send_failed++;
        }
        return;
    }

    if ((s_isr_tick_count % 500U) == 0U) {
        uint32_t val = s_isr_tick_count;
        queue_send_from_isr(&s_isr_queue, &val);
        event_set_from_isr(&s_isr_event, ISR_EVENT_BIT);
    }
}

static void keepalive_task(void)
{
    uint32_t counter = 0U;
    for (;;) {
        sched_delay_ms(1U);
        counter++;
        if ((counter % 1000U) == 0U) {
            uart_printf("[ALIVE] keepalive counter=%u, isr_tick=%u\r\n",
                        (unsigned int)counter, (unsigned int)s_isr_tick_count);
        }
    }
}

static void consumer_task(void)
{
    uart_puts("[TEST] Phase 1: waiting for ISR-signaled queue + event\r\n");

    systick_delay_ms(1000U);
    uart_printf("[DEBUG] isr_tick_count after 1s=%u (expect ~1000)\r\n",
                (unsigned int)s_isr_tick_count);

    for (uint8_t i = 0U; i < 3U; i++) {
        uint32_t before = systick_get_ms();

        uint32_t val;
        queue_receive(&s_isr_queue, &val);

        uint32_t after = systick_get_ms();
        uart_printf("[TEST] woke from queue, isr_tick=%u, latency=%ums\r\n",
                    (unsigned int)val, (unsigned int)(after - before));

        uint32_t bits = event_wait(&s_isr_event, ISR_EVENT_BIT, EVENT_WAIT_ANY, 1);
        uart_printf("[TEST] event observed, bits=0x%x\r\n", (unsigned int)bits);
    }

    uart_puts("\r\n[TEST] Phase 2: flooding queue_send_from_isr() faster than drain\r\n");
    s_flood_mode = 1U;

    for (uint8_t i = 0U; i < 20U; i++) {
        uint32_t val;
        queue_receive_from_isr(&s_isr_queue, &val); 
        systick_delay_ms(50U);
    }

    s_flood_mode = 0U;

    uart_printf("\r\n[TEST] flood results: send_ok=%u send_failed=%u\r\n",
                (unsigned int)s_isr_send_ok, (unsigned int)s_isr_send_failed);

    if (s_isr_send_failed > 0U) {
        uart_puts("[TEST] PASS: queue_send_from_isr() correctly returned -1 "
                   "under overflow instead of hanging the ISR\r\n");
    } else {
        uart_puts("[TEST] FAIL: expected some overflow failures, got none\r\n");
    }

    uart_puts("\r\n=== Done ===\r\n");
    for (;;) { }
}

int main(void)
{
    mcg_init_120mhz();
    systick_init(120000000UL);
    uart_init(115200U);

    uart_puts("TamgaOS K64F — ISR-Safe Primitives Test\r\n\r\n");

    queue_init(&s_isr_queue, s_isr_queue_buf, sizeof(uint32_t), 4U);
    event_init(&s_isr_event);

    sched_init();
    sched_task_create(consumer_task, 1U);
    sched_task_create(keepalive_task, 2U);
    sched_start();

    for (;;) { }
    return 0;
}