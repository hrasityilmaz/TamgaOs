/*
 * kalman.c — 4-state Kalman filter
 *
 * State: x = [roll, pitch, bias_gx, bias_gy]
 *
 * Predict:
 *   x = F*x + B*u   where u = [wx-bias_gx, wy-bias_gy]
 *   P = F*P*F' + Q
 *
 * Update:
 *   z = [roll_accel, pitch_accel]
 *   y = z - H*x
 *   S = H*P*H' + R
 *   K = P*H' * inv(S)
 *   x = x + K*y
 *   P = (I - K*H)*P
 *
 * F = [1  0  -dt  0 ]    B = [dt  0 ]
 *     [0  1   0  -dt]        [0   dt]
 *     [0  0   1   0 ]        [0   0 ]
 *     [0  0   0   1 ]        [0   0 ]
 *
 * H = [1 0 0 0]
 *     [0 1 0 0]
 */

#include "kalman.h"
#include <math.h>

/* Q — process noise */
/* Eksta step for P maybe some wind changed direction went up down etc... */
#define Q_ANGLE  0.001f
#define Q_BIAS   0.0001f

/* R — measurement noise from calibration (ADC^2 / 16384^2) */
/* from my imu came this data !! update here as needed */
#define SCALE    (1.0f / 16384.0f)
#define R_ROLL   (702.11f  * SCALE * SCALE)
#define R_PITCH  (651.55f  * SCALE * SCALE)

void kalman_init(kalman_t *k)
{
    for (int i = 0; i < 4; i++) {
        k->x[i] = 0.0f;
        for (int j = 0; j < 4; j++)
            k->P[i][j] = (i == j) ? 1.0f : 0.0f;
    }
}

void kalman_update(kalman_t *k, float ax, float ay, float az,
                   float wx, float wy, float dt)
{
    /* ── PREDICT ── */
    float roll  = k->x[0];
    float pitch = k->x[1];
    float bgx   = k->x[2];
    float bgy   = k->x[3];

    float wx_c = wx - bgx;
    float wy_c = wy - bgy;

    /* x = F*x + B*u */
    k->x[0] = roll  + wx_c * dt;
    k->x[1] = pitch + wy_c * dt;
    /* k->x[2] = bgx (unchanged) */
    /* k->x[3] = bgy (unchanged) */

    /* P = F*P*F' + Q
     * F = [1 0 -dt 0; 0 1 0 -dt; 0 0 1 0; 0 0 0 1]
     * Expanded (only non-trivial terms):
     */
    float P[4][4];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            P[i][j] = k->P[i][j];

    /* F*P rows */
    float FP[4][4];
    for (int j = 0; j < 4; j++) {
        FP[0][j] = P[0][j] - dt * P[2][j];
        FP[1][j] = P[1][j] - dt * P[3][j];
        FP[2][j] = P[2][j];
        FP[3][j] = P[3][j];
    }

    /* (F*P)*F' cols */
    float FPFT[4][4];
    for (int i = 0; i < 4; i++) {
        FPFT[i][0] = FP[i][0] - dt * FP[i][2];
        FPFT[i][1] = FP[i][1] - dt * FP[i][3];
        FPFT[i][2] = FP[i][2];
        FPFT[i][3] = FP[i][3];
    }

    /* P = FPFT + Q */
    k->P[0][0] = FPFT[0][0] + Q_ANGLE;
    k->P[1][1] = FPFT[1][1] + Q_ANGLE;
    k->P[2][2] = FPFT[2][2] + Q_BIAS;
    k->P[3][3] = FPFT[3][3] + Q_BIAS;
    /* off-diagonal */
    k->P[0][1] = FPFT[0][1]; k->P[1][0] = FPFT[1][0];
    k->P[0][2] = FPFT[0][2]; k->P[2][0] = FPFT[2][0];
    k->P[0][3] = FPFT[0][3]; k->P[3][0] = FPFT[3][0];
    k->P[1][2] = FPFT[1][2]; k->P[2][1] = FPFT[2][1];
    k->P[1][3] = FPFT[1][3]; k->P[3][1] = FPFT[3][1];
    k->P[2][3] = FPFT[2][3]; k->P[3][2] = FPFT[3][2];

    /* ── UPDATE ── */
    /* z = [roll_accel, pitch_accel] */
    float roll_meas  = atan2f(ay, sqrtf(ax*ax + az*az));
    float pitch_meas = atan2f(-ax, sqrtf(ay*ay + az*az));

    /* y = z - H*x  (H = [1 0 0 0; 0 1 0 0]) */
    float y0 = roll_meas  - k->x[0];
    float y1 = pitch_meas - k->x[1];

    /* S = H*P*H' + R = [P00+R_roll, P01; P10, P11+R_pitch] */
    float S00 = k->P[0][0] + R_ROLL;
    float S01 = k->P[0][1];
    float S10 = k->P[1][0];
    float S11 = k->P[1][1] + R_PITCH;

    /* inv(S) = 1/det * [S11 -S01; -S10 S00] */
    float det = S00 * S11 - S01 * S10;
    if (fabsf(det) < 1e-10f) return;
    float inv_det = 1.0f / det;

    float Si00 =  S11 * inv_det;
    float Si01 = -S01 * inv_det;
    float Si10 = -S10 * inv_det;
    float Si11 =  S00 * inv_det;

    /* K = P*H' * inv(S)
     * P*H' = [P00 P01; P10 P11; P20 P21; P30 P31]
     */
    float PH[4][2];
    PH[0][0] = k->P[0][0]; PH[0][1] = k->P[0][1];
    PH[1][0] = k->P[1][0]; PH[1][1] = k->P[1][1];
    PH[2][0] = k->P[2][0]; PH[2][1] = k->P[2][1];
    PH[3][0] = k->P[3][0]; PH[3][1] = k->P[3][1];

    float K[4][2];
    for (int i = 0; i < 4; i++) {
        K[i][0] = PH[i][0]*Si00 + PH[i][1]*Si10;
        K[i][1] = PH[i][0]*Si01 + PH[i][1]*Si11;
    }

    /* x = x + K*y */
    k->x[0] += K[0][0]*y0 + K[0][1]*y1;
    k->x[1] += K[1][0]*y0 + K[1][1]*y1;
    k->x[2] += K[2][0]*y0 + K[2][1]*y1;
    k->x[3] += K[3][0]*y0 + K[3][1]*y1;

    /* P = (I - K*H)*P */
    float KH[4][4] = {0};
    for (int i = 0; i < 4; i++) {
        KH[i][0] = K[i][0];
        KH[i][1] = K[i][1];
    }

    float IKH[4][4];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            IKH[i][j] = (i==j ? 1.0f : 0.0f) - KH[i][j];

    float newP[4][4] = {0};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int m = 0; m < 4; m++)
                newP[i][j] += IKH[i][m] * k->P[m][j];

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            k->P[i][j] = newP[i][j];
}