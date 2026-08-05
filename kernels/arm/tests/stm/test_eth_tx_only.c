/*
 * tests/test_eth_tx_only.c - sends a single Ethernet frame every
 * second in REAL mode (no loopback)
 */

#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "eth.h"
#include <string.h>

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();

    uart_puts("TamgaOS STM32H753ZI — Ethernet TX-Only Test\r\n\r\n");
    eth_init(0U); 
    uart_puts("[ETH] init done (real mode, no loopback)\r\n\r\n");

    uint8_t frame[64];
    memset(frame, 0, sizeof(frame));
    memset(&frame[0], 0xFFU, 6U);
    uint8_t src_mac[6] = { 0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U };
    memcpy(&frame[6], src_mac, 6U);
    frame[12] = 0x88U;
    frame[13] = 0xB5U;

    const char *msg = "TamgaOS!";
    memcpy(&frame[14], msg, 8U);

    uint32_t counter = 0U;

    for (;;) {
        frame[22] = (uint8_t)(counter & 0xFFU);

        // padding to 8 for alignment
        uint16_t padded_len = (23U + 3U) & ~3U;   /* 23 -> 24 */
        int8_t result = eth_transmit(frame, padded_len);

        if (result == 0) {
            uart_printf("[ETH] TX #%u sent OK\r\n", (unsigned int)counter);
        } else {
            uart_printf("[ETH] TX #%u FAILED\r\n", (unsigned int)counter);
        }

        counter++;
        systick_delay_ms(1000U);
    }

    return 0;
}