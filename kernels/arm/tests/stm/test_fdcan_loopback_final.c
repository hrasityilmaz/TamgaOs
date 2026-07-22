/*
 * test_fdcan_loopback_final.c
 *
 * Purpose: Confirm STM32's FDCAN1 driver works correctly in loopback
 * mode with ALL bug fixes applied (RCC offset, TXBC field, PSR
 * offset, NBTP field layout). Also lets you probe PB8/PB9 with a
 * logic analyzer to visually confirm a clean 1Mbit CAN frame pattern
 * even though nothing is physically wired to a transceiver.
 *
 * Expected UART log:
 *   [DEBUG] NBTP=0x401
 *   [DEBUG] TXBC=0x10018
 *   [DEBUG] wait_init(0) result=0, CCCR=0x0
 *   [DEBUG] PSR=... ACT=... (loopback doesn't need ACK, so this
 *           should NOT show Bus-Off — see note below)
 *   [TEST] sending ID=0x123 DLC=8 data=DE AD BE EF CA FE BA BE
 *   [TEST] received ID=0x123 DLC=8 data=DE AD BE EF CA FE BA BE
 *   [TEST] PASS
 *   (repeats every 500ms)
 *
 * On the logic analyzer, probe PB9 (TX) and PB8 (RX) — even in
 * loopback mode, the CAN core still drives the TX pin with the real
 * bit pattern (RM0433 confirms this for internal loopback... actually
 * check note in fdcan.c: internal loopback normally holds TX
 * recessive per RM0433, so seeing activity on PB9 here would
 * indicate something worth double-checking against that section —
 * either way, RX should show the FIFO successfully receiving what
 * TX intended).
 */

#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "fdcan.h"
#include <string.h>
#include <stdint.h>

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();
    fdcan_init();

    uart_puts("TamgaOS STM32H753ZI @ 480MHz\r\n");
    uart_puts("FDCAN1 Loopback Test (post-bugfix)\r\n\r\n");

    uint32_t counter = 0U;

    for (;;) {
        fdcan_frame_t tx;
        tx.id  = 0x123U;
        tx.dlc = 8U;
        tx.data[0] = 0xDE; tx.data[1] = 0xAD; tx.data[2] = 0xBE; tx.data[3] = 0xEF;
        tx.data[4] = 0xCA; tx.data[5] = 0xFE; tx.data[6] = 0xBA; tx.data[7] = 0xBE;

        uart_printf("[TEST] sending ID=0x%x DLC=8 data=DE AD BE EF CA FE BA BE\r\n",
                    (unsigned int)tx.id);

        int8_t tx_result = fdcan_transmit(&tx);
        if (tx_result != 0) {
            uart_puts("[TEST] FAIL: transmit timed out\r\n");
        } else {
            systick_delay_ms(10U);   /* give loopback path a moment */

            if (!fdcan_rx_pending()) {
                uart_puts("[TEST] FAIL: no frame received\r\n");
            } else {
                fdcan_frame_t rx;
                memset(&rx, 0, sizeof(rx));
                if (fdcan_receive(&rx) == 0) {
                    uart_printf("[TEST] received ID=0x%x DLC=%d data=",
                                (unsigned int)rx.id, (int)rx.dlc);
                    for (uint8_t i = 0U; i < rx.dlc; i++) {
                        uart_printf("%02X ", (unsigned int)rx.data[i]);
                    }
                    uart_puts("\r\n");

                    int match = (rx.id == tx.id) && (rx.dlc == tx.dlc);
                    for (uint8_t i = 0U; match && i < rx.dlc; i++) {
                        if (rx.data[i] != tx.data[i]) match = 0;
                    }

                    if (match) {
                        uart_puts("[TEST] PASS\r\n");
                    } else {
                        uart_puts("[TEST] FAIL: data mismatch\r\n");
                    }
                }
            }
        }

        uart_puts("\r\n");
        counter++;
        systick_delay_ms(500U);
    }

    return 0;
}