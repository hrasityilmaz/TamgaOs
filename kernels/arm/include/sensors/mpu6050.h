#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>

/* MPU6050 I2C address */
#define MPU6050_ADDR    0x68U   /* AD0=GND if not 0x69 !! and if no connection changing !!! */

typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t temp_raw;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
} mpu6050_data_t;

int8_t mpu6050_init(void);
int8_t mpu6050_read(mpu6050_data_t *data);

/* TODO: need to convert!!
 * why FPU needed :)   
 * accel: raw / 16384.0f = g  (±2g range)
 * gyro:  raw / 131.0f   = °/s (±250°/s range)
 * temp:  raw / 340.0f + 36.53f = °C
 */
#endif /* MPU6050_H */
