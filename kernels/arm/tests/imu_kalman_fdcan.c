/*
 * main.c — IMU → Kalman → FDCAN pipeline
 *
 * task_imu  (HIGH)   : MPU6050 read → Kalman → pack CAN frame → transmit
 * task_can  (NORMAL) : receive CAN frame → UART print
 * task_led  (LOW)    : heartbeat LED
 *
 * CAN frame layout (ID=0x100, DLC=8):
 *   byte[0..1] : roll  × 100  (int16_t, degrees × 100)
 *   byte[2..3] : pitch × 100  (int16_t, degrees × 100)
 *   byte[4..5] : gyro bias GX (int16_t, rad/s × 10000)
 *   byte[6..7] : gyro bias GY (int16_t, rad/s × 10000)
 */

#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "i2c.h"
#include "mpu6050.h"
#include "fdcan.h"
#include "kalman.h"
#include "scheduler.h"
#include "task.h"
#include <math.h>
#include <stdint.h>

#define RCC_AHB4ENR (*(volatile uint32_t *)0x580244E0U)
#define GPIOB_MODER (*(volatile uint32_t *)0x58020400U)
#define GPIOB_ODR   (*(volatile uint32_t *)0x58020414U)
#define LD1_PIN     (1UL << 0U)

#define CAN_ID_ATTITUDE  0x100U

static kalman_t g_kalman;

static void board_init(void)
{
    RCC_AHB4ENR |= (1UL << 1U);
    GPIOB_MODER &= ~(3UL << 0U);
    GPIOB_MODER |=  (1UL << 0U);
    GPIOB_ODR   &= ~LD1_PIN;
}

/* ── task_imu: sensor → Kalman → CAN TX ── */
void task_imu(void)
{
    mpu6050_data_t data;

    while (1) {
        sched_delay_ms(10U);   /* 100Hz */

        if (mpu6050_read(&data) != 0) continue;

        float ax = data.accel_x / 16384.0f;
        float ay = data.accel_y / 16384.0f;
        float az = data.accel_z / 16384.0f;
        float wx = data.gyro_x  / 131.0f * (3.14159265f / 180.0f);
        float wy = data.gyro_y  / 131.0f * (3.14159265f / 180.0f);

        kalman_update(&g_kalman, ax, ay, az, wx, wy, 0.01f);

        float roll_deg  = kalman_roll(&g_kalman)  * (180.0f / 3.14159265f);
        float pitch_deg = kalman_pitch(&g_kalman) * (180.0f / 3.14159265f);
        float bias_gx   = g_kalman.x[2];
        float bias_gy   = g_kalman.x[3];

        /* Pack into CAN frame — fixed-point encoding */
        int16_t roll_fp  = (int16_t)(roll_deg  * 100.0f);
        int16_t pitch_fp = (int16_t)(pitch_deg * 100.0f);
        int16_t bgx_fp   = (int16_t)(bias_gx   * 10000.0f);
        int16_t bgy_fp   = (int16_t)(bias_gy   * 10000.0f);

        fdcan_frame_t frame = {
            .id  = CAN_ID_ATTITUDE,
            .dlc = 8U,
            .data = {
                (uint8_t)( roll_fp        & 0xFF),
                (uint8_t)((roll_fp  >> 8) & 0xFF),
                (uint8_t)( pitch_fp       & 0xFF),
                (uint8_t)((pitch_fp >> 8) & 0xFF),
                (uint8_t)( bgx_fp         & 0xFF),
                (uint8_t)((bgx_fp   >> 8) & 0xFF),
                (uint8_t)( bgy_fp         & 0xFF),
                (uint8_t)((bgy_fp   >> 8) & 0xFF),
            }
        };

        fdcan_transmit(&frame);
    }
}

/* ── task_can: CAN RX → decode → UART ── */
void task_can(void)
{
    fdcan_frame_t frame;

    while (1) {
        sched_delay_ms(5U);

        if (!fdcan_rx_pending()) continue;
        if (fdcan_receive(&frame) != 0) continue;
        if (frame.id != CAN_ID_ATTITUDE || frame.dlc < 8U) continue;

        int16_t roll_fp  = (int16_t)(frame.data[0] | (frame.data[1] << 8));
        int16_t pitch_fp = (int16_t)(frame.data[2] | (frame.data[3] << 8));

        int roll_i  = roll_fp  / 100;
        int roll_f  = (roll_fp  < 0 ? -roll_fp  : roll_fp)  % 100;
        int pitch_i = pitch_fp / 100;
        int pitch_f = (pitch_fp < 0 ? -pitch_fp : pitch_fp) % 100;

        uart_printf("CAN[0x%x] R:%d.%d P:%d.%d\r\n",
                    frame.id, roll_i, roll_f, pitch_i, pitch_f);
    }
}

/* ── task_led: heartbeat ── */
void task_led(void)
{
    while (1) {
        GPIOB_ODR ^= LD1_PIN;
        sched_delay_ms(500U);
    }
}

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    board_init();
    uart_init();
    i2c_init();
    fdcan_init();

    uart_puts("TamgaOS STM32H753ZI @ 480MHz\r\n");
    uart_puts("IMU -> Kalman -> FDCAN pipeline\r\n");

    if (mpu6050_init() < 0) {
        uart_puts("MPU6050 init failed\r\n");
        while (1) {}
    }
    uart_puts("MPU6050 OK\r\n");

    kalman_init(&g_kalman);

    sched_init();

    sched_task_create(task_imu, TASK_PRIORITY_HIGH);
    sched_task_create(task_can, TASK_PRIORITY_NORMAL);
    sched_task_create(task_led, TASK_PRIORITY_LOW);

    sched_start();

    return 0;
}