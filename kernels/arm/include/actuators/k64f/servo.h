#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>

void servo_init(void);
void servo_set_angle(uint8_t channel, uint8_t angle_deg);
void servo_sweep_step(uint8_t channel);

#endif /* SERVO_H */