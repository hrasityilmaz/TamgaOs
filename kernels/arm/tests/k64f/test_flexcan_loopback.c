/*
 * test_flexcan_loopback.c
 *
 * Purpose: Prove that the FlexCAN0 driver (flexcan.c) correctly
 *          transmits and receives frames while in internal loopback
 *          mode (CTRL1.LPB=1, set by default in flexcan_init()).
 *          In loopback, a transmitted frame never touches the
 *          physical bus — it's routed straight back into the Rx
 *          mailbox by the peripheral itself. This lets us validate
 *          the encode -> transmit -> receive -> decode chain without
 *          needing a second board or a transceiver wired up yet.
 *
 * NOTE: This board's uart.c only provides uart_putc/uart_puts (no
 *       uart_printf), so this file includes small local helpers to
 *       print hex numbers without needing a full printf.
 *
 * ------------------------------------------------------------------
 * TEST 1 — Single frame round-trip
 * ------------------------------------------------------------------
 *   Send one frame with a known ID and known data pattern, then
 *   receive it back and verify every field matches exactly.
 *
 * ------------------------------------------------------------------
 * TEST 2 — Multiple sequential frames (proves the RX lock/unlock
 *           quirk via TIMER read is handled correctly — see the
 *           comment in flexcan_receive() in flexcan.c)
 * ------------------------------------------------------------------
 *   Send and receive 5 frames back-to-back with an incrementing
 *   payload. If the TIMER-register unlock were missing, only the
 *   FIRST frame would ever be received — this test would hang or
 *   fail on frame 2.
 *
 * ------------------------------------------------------------------
 * TEST 3 — Non-blocking rx_pending() check
 * ------------------------------------------------------------------
 *   Confirm flexcan_rx_pending() returns false on an empty Rx
 *   mailbox, then true immediately after a transmit.
 */

#include "mcg.h"
#include "pit.h"
#include "uart.h"
#include "flexcan.h"
#include <string.h>
#include <stdint.h>

/* ── Small local helpers: uart.c has no printf, so print numbers by hand ── */

static void uart_put_hex_nibble(uint8_t nibble)
{
    char c = (nibble < 10U) ? (char)('0' + nibble) : (char)('A' + (nibble - 10U));
    uart_putc(c);
}

static void uart_put_hex8(uint8_t v)
{
    uart_put_hex_nibble((uint8_t)(v >> 4U));
    uart_put_hex_nibble((uint8_t)(v & 0x0FU));
}

static void uart_put_hex32(uint32_t v)
{
    uart_put_hex8((uint8_t)(v >> 24U));
    uart_put_hex8((uint8_t)(v >> 16U));
    uart_put_hex8((uint8_t)(v >> 8U));
    uart_put_hex8((uint8_t)(v & 0xFFU));
}

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

static void simple_delay(volatile uint32_t n)
{
    while (n-- != 0U) {
        __asm volatile("nop");
    }
}

static void print_frame(const char *tag, const flexcan_frame_t *f)
{
    uart_puts(tag);
    uart_puts(" ID=0x");
    uart_put_hex32(f->id);
    uart_puts(" DLC=");
    uart_put_dec(f->dlc);
    uart_puts(" data=");
    for (uint8_t i = 0U; i < f->dlc; i++) {
        uart_put_hex8(f->data[i]);
        uart_putc(' ');
    }
    uart_puts("\r\n");
}

static int frames_equal(const flexcan_frame_t *a, const flexcan_frame_t *b)
{
    if (a->id != b->id) return 0;
    if (a->dlc != b->dlc) return 0;
    for (uint8_t i = 0U; i < a->dlc; i++) {
        if (a->data[i] != b->data[i]) return 0;
    }
    return 1;
}

/* ================= TEST 1 ================= */
static int test1_single_frame(void)
{
    flexcan_frame_t tx = {
        .id = 0x123U,
        .dlc = 8U,
        .data = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE}
    };

    print_frame("[TX]  sending", &tx);

    if (flexcan_transmit(&tx) != 0) {
        uart_puts("[TEST1] FAIL: transmit timed out\r\n");
        return 0;
    }

    simple_delay(1000U);   /* give the loopback path a moment */

    flexcan_frame_t rx;
    memset(&rx, 0, sizeof(rx));

    if (flexcan_receive(&rx) != 0) {
        uart_puts("[TEST1] FAIL: no frame received\r\n");
        return 0;
    }

    print_frame("[RX]  received", &rx);

    if (!frames_equal(&tx, &rx)) {
        uart_puts("[TEST1] FAIL: mismatch between sent and received frame\r\n");
        return 0;
    }

    uart_puts("[TEST1] PASS\r\n");
    return 1;
}

/* ================= TEST 2 ================= */
static int test2_sequential_frames(void)
{
    int all_ok = 1;

    for (uint8_t seq = 0U; seq < 5U; seq++) {
        flexcan_frame_t tx = {
            .id = 0x200U,
            .dlc = 8U,
            .data = {seq, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77}
        };

        uart_puts("[TX]  sending ID=0x200 DLC=8 seq=");
        uart_put_dec(seq);
        uart_puts("\r\n");

        if (flexcan_transmit(&tx) != 0) {
            uart_puts("[TEST2] FAIL: transmit timed out at seq=");
            uart_put_dec(seq);
            uart_puts("\r\n");
            all_ok = 0;
            break;
        }

        simple_delay(1000U);

        flexcan_frame_t rx;
        memset(&rx, 0, sizeof(rx));

        if (flexcan_receive(&rx) != 0) {
            uart_puts("[TEST2] FAIL: no frame received at seq=");
            uart_put_dec(seq);
            uart_puts(" (RX mailbox may be stuck locked)\r\n");
            all_ok = 0;
            break;
        }

        uart_puts("[RX]  received ID=0x200 DLC=8 seq=");
        uart_put_dec(rx.data[0]);
        uart_puts("\r\n");

        if (!frames_equal(&tx, &rx)) {
            uart_puts("[TEST2] FAIL: mismatch at seq=");
            uart_put_dec(seq);
            uart_puts("\r\n");
            all_ok = 0;
            break;
        }
    }

    if (all_ok) {
        uart_puts("[TEST2] PASS\r\n");
    }
    return all_ok;
}

/* ================= TEST 3 ================= */
static int test3_rx_pending(void)
{
    uint8_t pending_before = flexcan_rx_pending();
    uart_puts("[TEST3] rx_pending before send: ");
    uart_put_dec(pending_before);
    uart_puts(" (expected 0)\r\n");

    flexcan_frame_t tx = {
        .id = 0x300U,
        .dlc = 2U,
        .data = {0xAA, 0xBB}
    };

    if (flexcan_transmit(&tx) != 0) {
        uart_puts("[TEST3] FAIL: transmit timed out\r\n");
        return 0;
    }

    simple_delay(1000U);

    uint8_t pending_after = flexcan_rx_pending();
    uart_puts("[TEST3] rx_pending after send:  ");
    uart_put_dec(pending_after);
    uart_puts(" (expected 1)\r\n");

    if ((pending_before != 0U) || (pending_after == 0U)) {
        uart_puts("[TEST3] FAIL: unexpected pending state\r\n");
        return 0;
    }

    /* Drain the frame so it doesn't interfere with anything after this test */
    flexcan_frame_t rx;
    flexcan_receive(&rx);

    uart_puts("[TEST3] PASS\r\n");
    return 1;
}

int main(void)
{
    __asm volatile("cpsie i"); 
    mcg_init_120mhz();
    uart_init(115200U);

    uart_puts("Step 1: UART OK\r\n");   /* GEÇİCİ DEBUG */

    flexcan_init();

    uart_puts("Step 2: FlexCAN init OK\r\n");   /* GEÇİCİ DEBUG */

    uart_puts("TamgaOS K64F @ 120MHz\r\n");
    uart_puts("FlexCAN0 Loopback Test\r\n\r\n");

    int t1 = test1_single_frame();
    uart_puts("\r\n");
    int t2 = test2_sequential_frames();
    uart_puts("\r\n");
    int t3 = test3_rx_pending();
    uart_puts("\r\n");

    if (t1 && t2 && t3) {
        uart_puts("=== ALL FLEXCAN LOOPBACK TESTS PASSED ===\r\n");
    } else {
        uart_puts("=== SOME TESTS FAILED - see log above ===\r\n");
    }

    for (;;) {
        __asm volatile("wfi");
    }

    return 0;
}