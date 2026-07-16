/*
 * src/stm32h753zi/main.c — TamgaOS production entry point
 *
 * Boot sequence:
 *   1. Clocks (480MHz PLL1), SysTick, UART, I2C
 *   2. fault_log_init() + report any crash from the previous run
 *   3. iwdg_reset_was_watchdog() + report/clear the reset cause
 *   4. MPU6050 + Kalman filter init
 *   5. Arm IWDG (production safety net against a hung task)
 *   6. Start the scheduler with:
 *        - task_imu        (HIGH priority)   — sensor fusion, as before
 *        - task_housekeep  (LOW priority)    — kicks IWDG periodically
 *
 * WATCHDOG KICK DESIGN — READ BEFORE RELYING ON THIS IN THE FIELD:
 * task_housekeep() below only proves the SCHEDULER itself is still
 * alternating tasks; it does NOT prove task_imu (or any other future
 * task) is still making real progress. A task_imu that's stuck in an
 * infinite loop but not blocking the scheduler (e.g. spinning without
 * yielding is a different failure — that WOULD starve housekeep too,
 * so it's covered) would NOT be caught by this simple design if it's
 * merely looping over bad data while still yielding normally.
 *
 * For genuine "prove critical work is happening" coverage, each
 * critical task should periodically write a timestamp/heartbeat into
 * a shared state, and task_housekeep() should only call iwdg_kick()
 * if ALL monitored heartbeats have advanced recently — not just kick
 * unconditionally on its own schedule. That's a reasonable next step
 * once you have more than one critical task; keeping it simple here
 * since task_imu is still the only real workload.
 */

#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "i2c.h"
#include "mpu6050.h"
#include "kalman.h"
#include "iwdg.h"
#include "fault_log.h"
#include "scheduler.h"
#include "task.h"
#include <math.h>
#include <stdint.h>

static kalman_t g_kalman;

/* ── Boot-time diagnostics ── */
static void report_boot_diagnostics(void)
{
    fault_log_init();

    if (iwdg_reset_was_watchdog()) {
        uart_puts(">>> Previous reset cause: IWDG (watchdog timeout) <<<\r\n");
    }
    iwdg_clear_reset_flags();

    fault_log_t f;
    if (fault_log_check_and_clear(&f)) {
        uart_puts("=== Previous boot ended in a fault ===\r\n");
        uart_printf("EXC_RETURN=0x%x PC=0x%x LR=0x%x xPSR=0x%x\r\n",
                    f.exc_return, f.pc, f.lr, f.xpsr);
        uart_printf("CFSR=0x%x HFSR=0x%x\r\n", f.cfsr, f.hfsr);
        if (f.mmfar_valid) uart_printf("MMFAR=0x%x\r\n", f.mmfar);
        if (f.bfar_valid)  uart_printf("BFAR=0x%x\r\n", f.bfar);
        uart_puts("=======================================\r\n");
    }
}

/* ── Task: IMU read + Kalman fusion, 100Hz ── */
static void task_imu(void)
{
    mpu6050_data_t data;

    while (1) {
        sched_delay_ms(10U);   /* 100Hz */

        if (mpu6050_read(&data) != 0) continue;

        float ax = data.accel_x / 16384.0f;
        float ay = data.accel_y / 16384.0f;
        float az = data.accel_z / 16384.0f;
        float wx = data.gyro_x  / 131.0f * 3.14159265f / 180.0f;
        float wy = data.gyro_y  / 131.0f * 3.14159265f / 180.0f;

        kalman_update(&g_kalman, ax, ay, az, wx, wy, 0.01f);

        float roll  = kalman_roll(&g_kalman)  * 180.0f / 3.14159265f;
        float pitch = kalman_pitch(&g_kalman) * 180.0f / 3.14159265f;

        int roll_i  = (int)roll;
        int roll_f  = (int)(fabsf(roll  - (float)roll_i) * 100.0f);
        int pitch_i = (int)pitch;
        int pitch_f = (int)(fabsf(pitch - (float)pitch_i) * 100.0f);

        uart_printf("R:%d.%d P:%d.%d\r\n", roll_i, roll_f, pitch_i, pitch_f);
    }
}

/* ── Task: watchdog housekeeping, low priority ──
 * See the design-limitation comment at the top of this file before
 * treating this as a complete "system is healthy" proof — right now
 * it only proves the scheduler is still switching tasks.
 */
static void task_housekeep(void)
{
    while (1) {
        sched_delay_ms(200U);   /* well under the 1000ms IWDG timeout */
        iwdg_kick();
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

    report_boot_diagnostics();

    if (mpu6050_init() < 0) {
        uart_puts("MPU6050 init failed\r\n");
        while (1) {}
    }

    kalman_init(&g_kalman);

    /* Arm the watchdog before starting the scheduler, so a hang
       during task startup itself is also caught, not just hangs
       that happen once tasks are already running. */
    iwdg_init(1000U);

    sched_init();
    sched_task_create(task_imu, TASK_PRIORITY_HIGH);
    sched_task_create(task_housekeep, TASK_PRIORITY_LOW);
    sched_start();   /* never returns */

    return 0;
}