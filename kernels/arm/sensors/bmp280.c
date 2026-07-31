/*
 * bmp280.c — BMP280 barometric pressure/temperature sensor driver
 *
 * Uses I2C1 driver (i2c.c), same pattern as mpu6050.c.
 * Address: 0x76 (SDO=GND) — confirmed via I2C bus scan on this
 * project's board (see conversation: module was misidentified as
 * BMP180 initially, actually BMP280, address 0x76 not 0x77).
 *
 * Calibration + compensation formula per Bosch BMP280 datasheet,
 * Rev 1.23, section 3.11.3 (32-bit integer compensation, no
 * floating-point required for temp, float used only for the final
 * altitude conversion where FPU is already in use elsewhere in this
 * project).
 */

#include "bmp280.h"
#include "i2c.h"
#include "systick.h"
#include "uart.h"
#include <stdint.h>

#define BMP280_REG_CALIB_START  0x88U   /* 24 bytes */
#define BMP280_REG_CHIP_ID      0xD0U
#define BMP280_REG_RESET        0xE0U
#define BMP280_REG_CTRL_MEAS    0xF4U
#define BMP280_REG_CONFIG       0xF5U
#define BMP280_REG_DATA_START   0xF7U   /* press MSB/LSB/XLSB, temp MSB/LSB/XLSB — 6 bytes */

static uint16_t s_dig_T1;
static int16_t  s_dig_T2, s_dig_T3;
static uint16_t s_dig_P1;
static int16_t  s_dig_P2, s_dig_P3, s_dig_P4, s_dig_P5,
                s_dig_P6, s_dig_P7, s_dig_P8, s_dig_P9;

static int32_t s_reference_pressure_x256 = (int32_t)(101325U << 8U);

static int8_t bmp280_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    return i2c_write(BMP280_ADDR, buf, 2U);
}

static uint16_t le16_unsigned(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int16_t le16_signed(const uint8_t *p)
{
    return (int16_t)le16_unsigned(p);
}

int8_t bmp280_init(void)
{
    uint8_t chip_id = 0U;
    int8_t r = i2c_read(BMP280_ADDR, BMP280_REG_CHIP_ID, &chip_id, 1U);

    uart_printf("[BMP280-DEBUG] i2c_read result=%d chip_id=0x%x\r\n",
                (int)r, (unsigned int)chip_id);

    if (r < 0) {
        return -1;
    }
    if (chip_id != 0x58U) {
        return -2;   /* 0x58 = BMP280. 0x60 would be BME280 — if you
                        see 0x60 here, it's actually a BME280 and this
                        driver's chip-ID check needs updating, though
                        the rest of the register map is compatible. */
    }

    uint8_t cal[24];
    if (i2c_read(BMP280_ADDR, BMP280_REG_CALIB_START, cal, 24U) < 0) {
        return -1;
    }

    s_dig_T1 = le16_unsigned(&cal[0]);
    s_dig_T2 = le16_signed(&cal[2]);
    s_dig_T3 = le16_signed(&cal[4]);
    s_dig_P1 = le16_unsigned(&cal[6]);
    s_dig_P2 = le16_signed(&cal[8]);
    s_dig_P3 = le16_signed(&cal[10]);
    s_dig_P4 = le16_signed(&cal[12]);
    s_dig_P5 = le16_signed(&cal[14]);
    s_dig_P6 = le16_signed(&cal[16]);
    s_dig_P7 = le16_signed(&cal[18]);
    s_dig_P8 = le16_signed(&cal[20]);
    s_dig_P9 = le16_signed(&cal[22]);

    /* ctrl_meas: osrs_t=001 (x1), osrs_p=101 (x16), mode=11 (normal) */
    if (bmp280_write_reg(BMP280_REG_CTRL_MEAS, 0x57U) < 0) {
        return -1;
    }
    /* config: standby 62.5ms, filter off, no SPI 3-wire */
    if (bmp280_write_reg(BMP280_REG_CONFIG, 0x00U) < 0) {
        return -1;
    }

    return 0;
}

int8_t bmp280_read(bmp280_data_t *data)
{
    uint8_t buf[6];
    if (i2c_read(BMP280_ADDR, BMP280_REG_DATA_START, buf, 6U) < 0) {
        return -1;
    }

    int32_t adc_p = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | ((int32_t)buf[2] >> 4);
    int32_t adc_t = ((int32_t)buf[3] << 12) | ((int32_t)buf[4] << 4) | ((int32_t)buf[5] >> 4);

    /* --- Bosch's official 32-bit integer compensation formula --- */
    int32_t var1_t = ((((adc_t >> 3) - ((int32_t)s_dig_T1 << 1))) * (int32_t)s_dig_T2) >> 11;
    int32_t var2_t = (((((adc_t >> 4) - (int32_t)s_dig_T1) * ((adc_t >> 4) - (int32_t)s_dig_T1)) >> 12)
                      * (int32_t)s_dig_T3) >> 14;
    int32_t t_fine = var1_t + var2_t;

    int32_t temperature = (t_fine * 5 + 128) >> 8;   /* 0.01 degC units */

    int64_t var1_p = (int64_t)t_fine - 128000;
    int64_t var2_p = var1_p * var1_p * (int64_t)s_dig_P6;
    var2_p = var2_p + ((var1_p * (int64_t)s_dig_P5) << 17);
    var2_p = var2_p + (((int64_t)s_dig_P4) << 35);
    var1_p = ((var1_p * var1_p * (int64_t)s_dig_P3) >> 8) + ((var1_p * (int64_t)s_dig_P2) << 12);
    var1_p = (((((int64_t)1) << 47) + var1_p)) * ((int64_t)s_dig_P1) >> 33;

    int64_t pressure_x256;
    if (var1_p == 0) {
        pressure_x256 = 0;   /* avoid divide by zero */
    } else {
        int64_t p = 1048576 - adc_p;
        p = (((p << 31) - var2_p) * 3125) / var1_p;
        var1_p = (((int64_t)s_dig_P9) * (p >> 13) * (p >> 13)) >> 25;
        var2_p = (((int64_t)s_dig_P8) * p) >> 19;
        p = ((p + var1_p + var2_p) >> 8) + (((int64_t)s_dig_P7) << 4);
        pressure_x256 = p;
    }

    data->temperature_x100  = temperature;
    data->pressure_pa_x256  = (uint32_t)pressure_x256;

    float pressure_pa = (float)pressure_x256 / 256.0f;
    float ratio = pressure_pa / ((float)s_reference_pressure_x256 / 256.0f);
    data->altitude_m = 44330.0f * (1.0f - __builtin_powf(ratio, 0.1902949f));

    return 0;
}

int8_t bmp280_zero_altitude(uint8_t num_samples)
{
    if (num_samples == 0U) {
        num_samples = 1U;
    }

    int64_t sum = 0;
    uint8_t got = 0U;

    for (uint8_t i = 0U; i < num_samples; i++) {
        bmp280_data_t d;
        if (bmp280_read(&d) == 0) {
            sum += (int64_t)d.pressure_pa_x256;
            got++;
        }
        systick_delay_ms(50U);
    }

    if (got == 0U) {
        return -1;
    }

    s_reference_pressure_x256 = (int32_t)(sum / (int64_t)got);
    return 0;
}