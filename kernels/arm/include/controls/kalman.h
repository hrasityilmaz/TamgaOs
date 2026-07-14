#ifndef KALMAN_H
#define KALMAN_H

/*
 * kalman.h — 4-state Kalman filter for roll/pitch estimation
 *
 * State: x = [roll, pitch, bias_gx, bias_gy]
 * Input: gyro (wx, wy) in rad/s
 * Measurement: accel (ax, ay, az) in g
 *
 * R matrix from MPU6050 calibration (753 samples, static):
 *   var(ax) = 702.11 ADC^2  → R_roll  = 702.11 / 16384^2
 *   var(ay) = 651.55 ADC^2  → R_pitch = 651.55 / 16384^2
 */

#include <stdint.h>

typedef struct {
    float x[4];       /* state: [roll, pitch, bias_gx, bias_gy] */
    float P[4][4];    /* covariance matrix */
} kalman_t;

void  kalman_init(kalman_t *k);
void  kalman_update(kalman_t *k, float ax, float ay, float az,
                    float wx, float wy, float dt);

static inline float kalman_roll(const kalman_t *k)  { return k->x[0]; }
static inline float kalman_pitch(const kalman_t *k) { return k->x[1]; }

#endif /* KALMAN_H */