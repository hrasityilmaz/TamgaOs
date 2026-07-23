#ifndef PWM_H
#define PWM_H

#include <stdint.h>

void pwm_init(void);
void pwm_set_pulse_us(uint16_t pulse_us);
void pwm_debug_print_regs(void);
void pwm_debug_gpio_toggle_test(void);

#endif /* PWM_H */