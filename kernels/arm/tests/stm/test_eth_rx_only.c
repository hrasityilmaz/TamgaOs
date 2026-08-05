/*
 * tests/test_eth_rx_only.c - tests REAL RX 
 */

#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "eth.h"

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();
    uart_puts("TamgaOS STM32H753ZI — Ethernet RX-Only Test (real mode)\r\n\r\n");

    eth_init(0U);
    uart_puts("[ETH] init done\r\n\r\n");

    eth_debug_dump_registers();

    uint8_t rx_buf[64];
    uint16_t rx_len = 0U;
    uint32_t rx_count = 0U;
    uint32_t our_frame_count = 0U;

    for (;;) {
        int8_t result = eth_receive(rx_buf, sizeof(rx_buf), &rx_len);

        if (result == 0) {
            rx_count++;
            uint16_t ethertype = 0U;
            if (rx_len >= 14U) {
                ethertype = (uint16_t)(((uint16_t)rx_buf[12] << 8) | rx_buf[13]);
            }

            if (ethertype == 0x88B5U) {
                our_frame_count++;
                uart_printf("[ETH] *** TAMGA TEST *** #%u (rx total #%u) — %u bytes: ",
                            (unsigned int)our_frame_count, (unsigned int)rx_count,
                            (unsigned int)rx_len);
                for (uint16_t i = 0U; i < rx_len && i < 32U; i++) {
                    uart_printf("%02X ", (unsigned int)rx_buf[i]);
                }
                uart_puts("\r\n");
                uart_puts("[ETH]   payload ascii: ");
                for (uint16_t i = 14U; i < rx_len && i < 32U; i++) {
                    char c = (char)rx_buf[i];
                    uart_putc((c >= 32 && c < 127) ? c : '.');
                }
                uart_puts("\r\n");
            } else {
                uart_printf("[ETH] rx #%u (other traffic, ethertype=0x%X, %u bytes)\r\n",
                            (unsigned int)rx_count, (unsigned int)ethertype,
                            (unsigned int)rx_len);
            }
        }

        systick_delay_ms(50U);
    }

    return 0;
}