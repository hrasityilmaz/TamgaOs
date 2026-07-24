/*
 * tests/stm/adc_test.c — reads PA3 (Arduino A0, ADC1_INP15)
 * continuously and prints the raw value over UART every 200ms.
 */

#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "adc.h"

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();

    uart_puts("TamgaOS STM32H753ZI @ 480MHz\r\n");
    uart_puts("ADC Test — PA3 (A0) / ADC1_INP15\r\n\r\n");

    adc_init();
    uart_puts("[ADC] init done\r\n\r\n");

    for (;;) {
        uint16_t raw = adc_read_channel15();
        uart_printf("[ADC] PA3 raw=%u\r\n", (unsigned int)raw);
        systick_delay_ms(200U);
    }

    return 0;
}