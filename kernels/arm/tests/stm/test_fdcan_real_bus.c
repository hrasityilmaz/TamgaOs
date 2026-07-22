#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "fdcan.h"
#include <string.h>
#include <stdint.h>

#define MY_TX_ID (0x100U)

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();
    fdcan_init();
    uint32_t psr = fdcan_get_psr();
    uint32_t act = (psr >> 16U) & 0x3U;
    uart_printf("[DEBUG] PSR=0x%x ACT=%d (0=Sync,1=Idle,2=Rx,3=Tx)\r\n",
                (unsigned int)psr, (int)act);
   fdcan_debug_print_gpio();

    uart_puts("TamgaOS STM32H753ZI @ 480MHz\r\n");
    uart_puts("FDCAN1 Real Bus Test (TX ID=0x100)\r\n\r\n");

    uint32_t counter = 0U;
    uint32_t last_tx_ms = 0U;

    for (;;) {
        uint32_t now = systick_get_ms();

        if ((now - last_tx_ms) >= 500U) {
            last_tx_ms = now;

            fdcan_frame_t tx;
            tx.id  = MY_TX_ID;
            tx.dlc = 8U;
            memset(tx.data, 0, sizeof(tx.data));
            tx.data[0] = (uint8_t)(counter & 0xFFU);
            tx.data[1] = (uint8_t)((counter >> 8U) & 0xFFU);

            int8_t result = fdcan_transmit(&tx);
            if (result == 0) {
                uart_printf("[TX] sent ID=0x%x counter=%d\r\n",
                            (unsigned int)tx.id, (int)counter);
            } else {
                uart_printf("[TX] FAILED (no ACK?) counter=%d\r\n", (int)counter);
            }
            counter++;

            if (fdcan_is_bus_off()) {
                uart_puts("[CAN] BUS-OFF detected, reinitializing...\r\n");
                fdcan_init();
            }
        }

        if (fdcan_rx_pending()) {
            fdcan_frame_t rx;
            if (fdcan_receive(&rx) == 0) {
                uart_printf("[RX] ID=0x%x DLC=%d data=",
                            (unsigned int)rx.id, (int)rx.dlc);
                for (uint8_t i = 0U; i < rx.dlc; i++) {
                    uart_printf("%02X ", (unsigned int)rx.data[i]);
                }
                uart_puts("\r\n");
            }
        }
    }

    return 0;
}