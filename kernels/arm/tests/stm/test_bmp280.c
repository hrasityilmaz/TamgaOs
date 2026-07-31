/*
 * tests/test_bmp280.c — BMP280 altitude test with zero-reference and
 * decimal altitude display (integer truncation was hiding sub-meter
 * changes — 0.9m was printing as "0m", same class of bug as
 * throwing away a fractional part instead of rounding).
 */

#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "i2c.h"
#include "bmp280.h"

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();
    i2c_init();

    uart_puts("TamgaOS STM32H753ZI — BMP280 Test\r\n\r\n");

    int8_t err = bmp280_init();
    if (err != 0) {
        uart_printf("[BMP280] init FAILED, err=%d\r\n", (int)err);
        for (;;) { }
    }
    uart_puts("[BMP280] init OK\r\n");

    uart_puts("[BMP280] zeroing altitude reference...\r\n");
    bmp280_zero_altitude(10U);
    uart_puts("[BMP280] altitude zeroed — current position = 0m\r\n\r\n");

    for (;;) {
        bmp280_data_t data;
        if (bmp280_read(&data) == 0) {
            int32_t temp_int  = data.temperature_x100 / 100;
            int32_t temp_frac = data.temperature_x100 % 100;
            uint32_t pressure_pa = data.pressure_pa_x256 / 256U;

            /* Decimal altitude display — truncating straight to
             * int32_t was hiding sub-meter movement entirely, since
             * anything under 1.0m (or -1.0m to 0.0m) collapsed to
             * "0m" regardless of the actual fractional value. */
            int32_t alt_x10 = (int32_t)(data.altitude_m * 10.0f);
            int32_t alt_int_part  = alt_x10 / 10;
            int32_t alt_frac_part = alt_x10 % 10;
            if (alt_frac_part < 0) alt_frac_part = -alt_frac_part;

            uart_printf("[BMP280] temp=%d.%02dC pressure=%uPa altitude=%d.%dm\r\n",
                        (int)temp_int, (int)temp_frac,
                        (unsigned int)pressure_pa,
                        (int)alt_int_part, (int)alt_frac_part);
        } else {
            uart_puts("[BMP280] read FAILED\r\n");
        }

        systick_delay_ms(1000U);
    }

    return 0;
}