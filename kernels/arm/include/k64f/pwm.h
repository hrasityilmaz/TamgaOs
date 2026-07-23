#ifndef PWM_H
#define PWM_H

#include <stdint.h>

/*
 * pwm.h — K64F FTM0/FTM3 4-channel PWM API
 *
 * Motor 1 = channel 0 (PTC1, FTM0_CH0)
 * Motor 2 = channel 1 (PTC5, FTM0_CH2)
 * Motor 3 = channel 2 (PTC8, FTM3_CH4)
 * Motor 4 = channel 3 (PTC9, FTM3_CH5)
 */

void pwm_init(void);
void pwm_set_pulse_us_ch(uint8_t channel, uint16_t pulse_us);
void pwm_debug_print_regs(void);

#endif /* PWM_H */