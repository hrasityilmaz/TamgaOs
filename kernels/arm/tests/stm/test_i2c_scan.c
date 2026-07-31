/*
 * tests/test_i2c_scan.c — scans all valid 7-bit I2C addresses
 * (0x08-0x77, the conventional usable range excluding reserved
 * addresses) and reports which ones ACK, i.e. which ones have a
 * device physically present and responding.
 *
 * Uses i2c_read(addr, 0x00, &dummy, 1) as the probe — this issues a
 * standard "write register pointer 0x00, repeated start, read 1
 * byte" transaction. What register 0x00 actually means to the
 * device doesn't matter for this purpose; what matters is whether
 * the device ACKs its own address at all. This is the same approach
 * most Arduino Wire.h I2C scanners use.
 *
 * Expected results on a healthy bus:
 *   MPU6050  -> 0x68 (AD0=GND) or 0x69 (AD0=VCC)
 *   BMP180   -> 0x77
 * If BMP180 doesn't show up here at all while MPU6050 does, the
 * problem is confirmed to be specific to BMP180 (wiring, module
 * damage, or address) rather than the I2C bus itself.
 */

#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "i2c.h"

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();
    i2c_init();

    uart_puts("TamgaOS STM32H753ZI — I2C Bus Scanner\r\n\r\n");
    uart_puts("Scanning addresses 0x08-0x77...\r\n\r\n");

    uint8_t found_count = 0U;

    for (uint8_t addr = 0x08U; addr <= 0x77U; addr++) {
        uint8_t dummy = 0U;
        int8_t result = i2c_read(addr, 0x00U, &dummy, 1U);

        if (result >= 0) {
            uart_printf("[SCAN] device found at 0x%x\r\n", (unsigned int)addr);
            found_count++;
        }

        systick_delay_ms(5U);   /* small gap between probes, generous
                                    margin, not performance-critical */
    }

    uart_printf("\r\n[SCAN] complete — %u device(s) found\r\n",
                (unsigned int)found_count);

    if (found_count == 0U) {
        uart_puts("[SCAN] No devices responded at all — check wiring,\r\n");
        uart_puts("       power, and GND before suspecting individual sensors.\r\n");
    }

    for (;;) { }
    return 0;
}