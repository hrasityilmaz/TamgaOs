/*
 * mpu6050.c — MPU6050 IMU driver
 *
 * Uses I2C1 driver (i2c.c)
 * Address: 0x69 (AD0=VCC) or 0x68 (AD0=GND)
 *
 * Registers:
 *   0x6B PWR_MGMT_1  — wake up
 *   0x1A CONFIG      — DLPF
 *   0x1B GYRO_CONFIG — ±250°/s
 *   0x1C ACCEL_CONFIG— ±2g
 *   0x3B ACCEL_XOUT_H — data start (14 bytes total)
 *   0x75 WHO_AM_I    — should return 0x68
 *
 * REF: MPU-6000/6050 Register Map Rev 4.2
 */

#include "mpu6050.h"
#include "i2c.h"
#include <stdint.h>

#define MPU6050_REG_WHO_AM_I     0x75U
#define MPU6050_REG_PWR_MGMT_1   0x6BU
#define MPU6050_REG_CONFIG       0x1AU
#define MPU6050_REG_GYRO_CONFIG  0x1BU
#define MPU6050_REG_ACCEL_CONFIG 0x1CU
#define MPU6050_REG_ACCEL_XOUT_H 0x3BU

static int8_t mpu6050_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    return i2c_write(MPU6050_ADDR, buf, 2U);
}

// Init
int8_t mpu6050_init(void)
{
    uint8_t who = 0U;
    if (i2c_read(MPU6050_ADDR, MPU6050_REG_WHO_AM_I, &who, 1U) < 0) return -1;
    if (who != 0x68U) return -2;
    if (mpu6050_write_reg(MPU6050_REG_PWR_MGMT_1, 0x00U) < 0) return -1;
    if (mpu6050_write_reg(MPU6050_REG_CONFIG, 0x03U) < 0) return -1;
    if (mpu6050_write_reg(MPU6050_REG_GYRO_CONFIG, 0x00U) < 0) return -1;
    if (mpu6050_write_reg(MPU6050_REG_ACCEL_CONFIG, 0x00U) < 0) return -1;

    return 0;
}

// read
int8_t mpu6050_read(mpu6050_data_t *data)
{
    uint8_t buf[14];
    if (i2c_read(MPU6050_ADDR, MPU6050_REG_ACCEL_XOUT_H, buf, 14U) < 0) return -1;

    data->accel_x  = (int16_t)((buf[0]  << 8) | buf[1]);
    data->accel_y  = (int16_t)((buf[2]  << 8) | buf[3]);
    data->accel_z  = (int16_t)((buf[4]  << 8) | buf[5]);
    data->temp_raw = (int16_t)((buf[6]  << 8) | buf[7]);
    data->gyro_x   = (int16_t)((buf[8]  << 8) | buf[9]);
    data->gyro_y   = (int16_t)((buf[10] << 8) | buf[11]);
    data->gyro_z   = (int16_t)((buf[12] << 8) | buf[13]);

    return 0;
}
