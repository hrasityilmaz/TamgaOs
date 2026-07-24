#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>

/*
 * servo.h — generic hobby servo abstraction, built on top of pwm.c
 * (drivers/stm32h753zi/pwm.c, TIM2_CH1/PA0).
 */

void servo_init(void);
void servo_set_angle(uint8_t angle_deg);
void servo_sweep_step(void);

#endif /* SERVO_H */