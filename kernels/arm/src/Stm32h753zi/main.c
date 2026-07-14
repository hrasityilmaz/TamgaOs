#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "scheduler.h"
#include "task.h"
#include "i2c.h"
#include "mpu6050.h"
#include "kalman.h"
#include <math.h>
#include <stdint.h>

static kalman_t g_kalman;
float gx_bias = 0, gy_bias = 0;
float ax_bias = 0, ay_bias = 0;

void task_imu(void)
{
    mpu6050_data_t data;

    while (1) {
        sched_delay_ms(10U);   /* 100Hz */

        if (mpu6050_read(&data) != 0) continue;

        float ax = (data.accel_x - ax_bias) / 16384.0f;
        float ay = (data.accel_y - ay_bias) / 16384.0f;
        float az = data.accel_z / 16384.0f;   /* Z ekseni bias'ı dokunulmaz, 1g referansı içeriyor */
        float wx = (data.gyro_x - gx_bias) / 131.0f * 3.14159265f / 180.0f;
        float wy = (data.gyro_y - gy_bias) / 131.0f * 3.14159265f / 180.0f;

        kalman_update(&g_kalman, ax, ay, az, wx, wy, 0.01f);

        float roll  = kalman_roll(&g_kalman)  * 180.0f / 3.14159265f;
        float pitch = kalman_pitch(&g_kalman) * 180.0f / 3.14159265f;

        int roll_i  = (int)roll;
        int roll_f  = (int)(fabsf(roll  - (float)roll_i) * 100.0f);
        int pitch_i = (int)pitch;
        int pitch_f = (int)(fabsf(pitch - (float)pitch_i) * 100.0f);

        uart_printf("R:%d.%d P:%d.%d\r\n",
            roll_i, roll_f, pitch_i, pitch_f);
    }
}

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();
    i2c_init();

    uart_puts("TamgaOS STM32H753ZI @ 480MHz\r\n");
    uart_puts("Kalman roll/pitch\r\n");

    if (mpu6050_init() < 0) {
        uart_puts("MPU6050 init failed\r\n");
        while (1) {}
    }

    /* Calibration.. sensor must be still and level during this */
    mpu6050_data_t d;
    uart_puts("Calibrating, keep still...\r\n");
    for (int i = 0; i < 500; i++) {
        if (mpu6050_read(&d) == 0) {
            gx_bias += d.gyro_x;
            gy_bias += d.gyro_y;
            ax_bias += d.accel_x;
            ay_bias += d.accel_y;
        }
        systick_delay_ms(2U);
    }
    gx_bias /= 500.0f;
    gy_bias /= 500.0f;
    ax_bias /= 500.0f;
    ay_bias /= 500.0f;

    kalman_init(&g_kalman);

    sched_init();
    sched_task_create(task_imu, TASK_PRIORITY_HIGH);
    sched_start();

    return 0;
}