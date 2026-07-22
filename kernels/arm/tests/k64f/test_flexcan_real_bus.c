/*
 * test_flexcan_real_bus.c (K64F)
 *
 * Purpose: Real two-board CAN bus test (no loopback). This board
 * (K64F) periodically transmits a frame with ID=0x200 and an
 * incrementing counter, and continuously polls for any incoming
 * frame, printing whatever it receives (expected: STM32's frames at
 * ID=0x100).
 *
 * Prerequisites:
 *   - flexcan.c: CAN_CTRL1_LPB_MASK removed from the CTRL1 write
 *     (real bus mode, not loopback)
 *   - PTB18=TX/PTB19=RX wired to a CAN transceiver, CANH/CANL tied
 *     to the STM32's transceiver bus (straight, not crossed)
 *   - STM32 running the matching tests/test_fdcan_real_bus.c
 */

#include "mcg.h"
#include "systick.h"
#include "uart.h"
#include "flexcan.h"
#include <string.h>
#include <stdint.h>

#define MY_TX_ID (0x200U)

static void uart_put_hex8(uint8_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    uart_putc(hex[(v >> 4U) & 0xFU]);
    uart_putc(hex[v & 0xFU]);
}

static void uart_put_hex32(uint32_t v)
{
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t nibble = (uint8_t)((v >> i) & 0xFU);
        uart_putc(nibble < 10U ? (char)('0' + nibble) : (char)('A' + nibble - 10U));
    }
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

int main(void)
{
    mcg_init_120mhz();
    uart_init(115200U);
    flexcan_init();
    systick_init(120000000UL);

    uart_puts("TamgaOS K64F @ 120MHz\r\n");
    uart_puts("FlexCAN0 Real Bus Test (TX ID=0x200)\r\n\r\n");

    uint32_t counter = 0U;
    uint32_t last_tx_ms = 0U;

    for (;;) {
        uint32_t now = systick_get_ms();

        if ((now - last_tx_ms) >= 500U) {
            last_tx_ms = now;

            flexcan_frame_t tx;
            tx.id  = MY_TX_ID;
            tx.dlc = 8U;
            memset(tx.data, 0, sizeof(tx.data));
            tx.data[0] = (uint8_t)(counter & 0xFFU);
            tx.data[1] = (uint8_t)((counter >> 8U) & 0xFFU);

            int8_t result = flexcan_transmit(&tx);
            if (result == 0) {
                uart_puts("[TX] sent ID=0x");
                uart_put_hex32(tx.id);
                uart_puts(" counter=");
                uart_put_dec(counter);
                uart_puts("\r\n");
            } else {
                uart_puts("[TX] FAILED (no ACK?) counter=");
                uart_put_dec(counter);
                uart_puts("\r\n");
            }
            counter++;
        }

        if (flexcan_rx_pending()) {
            flexcan_frame_t rx;
            if (flexcan_receive(&rx) == 0) {
                uart_puts("[RX] ID=0x");
                uart_put_hex32(rx.id);
                uart_puts(" DLC=");
                uart_put_dec(rx.dlc);
                uart_puts(" data=");
                for (uint8_t i = 0U; i < rx.dlc; i++) {
                    uart_put_hex8(rx.data[i]);
                    uart_putc(' ');
                }
                uart_puts("\r\n");
            }
        }
    }

    return 0;
}