/*
 * servo.c — generic hobby servo control, built on pwm.c's TIM2_CH1/PA0 driver. 
 */

#include "servo.h"
#include "pwm.h"
#include "uart.h" 

#define SERVO_PULSE_MIN_US   1000U
#define SERVO_PULSE_MAX_US   2000U
#define SERVO_ANGLE_MAX_DEG  180U

static uint8_t s_sweep_angle = 0U;
static int8_t  s_sweep_dir   = 1;
#define SERVO_SWEEP_STEP_DEG  1U   /* 2 or 3 more fast will be */

void servo_init(void)
{
    pwm_init();
    servo_set_angle(90U);
}

void servo_set_angle(uint8_t angle_deg)
{
    if (angle_deg > SERVO_ANGLE_MAX_DEG) {
        angle_deg = SERVO_ANGLE_MAX_DEG;
    }
    uint32_t span_us = SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US;
    uint16_t pulse_us = (uint16_t)(SERVO_PULSE_MIN_US +
                         ((uint32_t)angle_deg * span_us) / SERVO_ANGLE_MAX_DEG);

    pwm_set_pulse_us(pulse_us);
}

void servo_sweep_step(void)
{
    servo_set_angle(s_sweep_angle);
    uart_printf("[SERVO] angle=%d deg (dir=%s)\r\n", (int)s_sweep_angle, (s_sweep_dir > 0) ? "up" : "down");

    if (s_sweep_angle >= SERVO_ANGLE_MAX_DEG) {
        s_sweep_dir = -1;
    } else if (s_sweep_angle == 0U) {
        s_sweep_dir = 1;
    }
    s_sweep_angle = (uint8_t)(s_sweep_angle + (s_sweep_dir * SERVO_SWEEP_STEP_DEG));
}



