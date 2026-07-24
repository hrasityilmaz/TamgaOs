/*
 * servo.c — generic hobby servo/ESC control, K64F build.
 * Built on drivers/k64f/pwm.c (FTM0/FTM3, 4 channels: PTC1/PTC5/PTC8/PTC9).
 *
 */

#include "servo.h"
#include "pwm.h"

#define SERVO_PULSE_MIN_US   1000U
#define SERVO_PULSE_MAX_US   2000U
#define SERVO_ANGLE_MAX_DEG  180U
#define SERVO_SWEEP_STEP_DEG  1U
#define SERVO_NUM_CHANNELS    4U

static uint8_t s_sweep_angle[SERVO_NUM_CHANNELS] = {0U, 0U, 0U, 0U};
static int8_t  s_sweep_dir[SERVO_NUM_CHANNELS]   = {1, 1, 1, 1};

void servo_init(void)
{
    pwm_init();

    for (uint8_t ch = 0U; ch < SERVO_NUM_CHANNELS; ch++) {
        servo_set_angle(ch, 90U);
    }
}

void servo_set_angle(uint8_t channel, uint8_t angle_deg)
{
    if (channel >= SERVO_NUM_CHANNELS) {
        return;
    }
    if (angle_deg > SERVO_ANGLE_MAX_DEG) {
        angle_deg = SERVO_ANGLE_MAX_DEG;
    }

    uint32_t span_us = SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US;
    uint16_t pulse_us = (uint16_t)(SERVO_PULSE_MIN_US +
                         ((uint32_t)angle_deg * span_us) / SERVO_ANGLE_MAX_DEG);

    pwm_set_pulse_us_ch(channel, pulse_us);
}

void servo_sweep_step(uint8_t channel)
{
    if (channel >= SERVO_NUM_CHANNELS) {
        return;
    }

    servo_set_angle(channel, s_sweep_angle[channel]);

    if (s_sweep_angle[channel] >= SERVO_ANGLE_MAX_DEG) {
        s_sweep_dir[channel] = -1;
    } else if (s_sweep_angle[channel] == 0U) {
        s_sweep_dir[channel] = 1;
    }

    s_sweep_angle[channel] = (uint8_t)(s_sweep_angle[channel] +
                              (s_sweep_dir[channel] * SERVO_SWEEP_STEP_DEG));
}