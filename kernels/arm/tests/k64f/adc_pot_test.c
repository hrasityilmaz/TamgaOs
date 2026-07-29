/*
 * tests/k64f/adc_pot_test.c  reads a potentiometer wired to A0
 * (PTB2, ADC0_SE12) continuously and prints the raw 12-bit value
 * over UART every 200ms.
 */

#include "mcg.h"
#include "systick.h"
#include "uart.h"
#include "adc.h"

int main(void)
{
    mcg_init_120mhz();
    systick_init(120000000UL);
    uart_init(115200U);

    uart_puts("TamgaOS K64F — ADC Test — A0 (PTB2) / ADC0_SE12\r\n\r\n");

    adc_init();

    if (adc_calibration_failed()) {
        uart_puts("[ADC] WARNING: calibration failed (CALF set) — "
                   "readings may be inaccurate\r\n\r\n");
    } else {
        uart_puts("[ADC] calibration OK\r\n\r\n");
    }

    for (;;) {
        uint16_t raw = adc_read_se12();
        uart_printf("[ADC] A0 raw=%u\r\n", (unsigned int)raw);
        systick_delay_ms(200U);
    }

    return 0;
}