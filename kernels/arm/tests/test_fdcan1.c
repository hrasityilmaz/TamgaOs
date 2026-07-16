#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "fdcan.h"
#include <stdint.h>

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();
    fdcan_init();

    uart_puts("TamgaOS STM32H753ZI @ 480MHz\r\n");
    uart_puts("FDCAN1 loopback test\r\n");

    fdcan_frame_t tx_frame = {
        .id  = 0x123U,
        .dlc = 8U,
        .data = { 0x54, 0x61, 0x6D, 0x67, 0x61, 0x4F, 0x53, 0x21 }
        /* "TamgaOS!" */
    };

    uint8_t pass = 0U;
    uint8_t fail = 0U;

    for (uint8_t i = 0U; i < 10U; i++) {
        tx_frame.data[0] = i;

        if (fdcan_transmit(&tx_frame) < 0) {
            uart_printf("[%d] TX error\r\n", i);
            fail++;
            continue;
        }

        /* Wait for loopback */
        uint32_t timeout = 100000U;
        while (!fdcan_rx_pending() && timeout--) {}

        if (!fdcan_rx_pending()) {
            uart_printf("[%d] RX timeout\r\n", i);
            fail++;
            continue;
        }

        fdcan_frame_t rx_frame;
        if (fdcan_receive(&rx_frame) < 0) {
            uart_printf("[%d] RX error\r\n", i);
            fail++;
            continue;
        }

        if (rx_frame.id == tx_frame.id && rx_frame.data[0] == i) {
            uart_printf("[%d] OK  ID=0x%x data[0]=%d\r\n", i, rx_frame.id, rx_frame.data[0]);
            pass++;
        } else {
            uart_printf("[%d] FAIL ID=0x%x data[0]=%d\r\n", i, rx_frame.id, rx_frame.data[0]);
            fail++;
        }

        systick_delay_ms(100U);
    }

    uart_printf("\nResult: %d/10 pass, %d fail\r\n", pass, fail);

    while (1) {}
}