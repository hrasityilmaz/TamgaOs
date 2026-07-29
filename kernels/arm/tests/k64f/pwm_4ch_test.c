/*
 * tests/k64f/pwm_4ch_test.c 
 *
 * Channel map:
 *   0 = Motor 1 (PTC1, FTM0_CH0)
 *   1 = Motor 2 (PTC5, FTM0_CH2)
 *   2 = Motor 3 (PTC8, FTM3_CH4)
 *   3 = Motor 4 (PTC9, FTM3_CH5)
 */

#include "mcg.h"
#include "systick.h"
#include "uart.h"
#include "pwm.h"

#define NUM_CHANNELS  4U
#define PULSE_MIN     1000U
#define PULSE_MAX     2000U
#define PULSE_STEP    100U
#define STEP_DELAY_MS 200U

int main(void)
{
    mcg_init_120mhz();
    systick_init(120000000UL);
    uart_init(115200U);

    uart_puts("TamgaOS K64F — 4-Channel PWM Test (all motors together)\r\n");

    pwm_init();

    uart_puts("[PWM4] Channels: 0=PTC1 1=PTC5 2=PTC8 3=PTC9\r\n");
    uart_puts("[PWM4] Sweeping all 4 channels together, 1000-2000us\r\n\r\n");

    uint16_t pulse = PULSE_MIN;
    int8_t   direction = 1;

    for (;;) {
        for (uint8_t ch = 0U; ch < NUM_CHANNELS; ch++) {
            pwm_set_pulse_us_ch(ch, pulse);
        }

        uart_printf("[PWM4] ch0=%uus ch1=%uus ch2=%uus ch3=%uus\r\n",
                    (unsigned int)pulse, (unsigned int)pulse,
                    (unsigned int)pulse, (unsigned int)pulse);

        pulse = (uint16_t)(pulse + (direction * PULSE_STEP));
        if (pulse >= PULSE_MAX) direction = -1;
        if (pulse <= PULSE_MIN) direction = 1;

        systick_delay_ms(STEP_DELAY_MS);
    }

    return 0;
}