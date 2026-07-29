/*
 * tests/k64f/pwm_4ch_pot_test.c
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
#include "adc.h"

#define NUM_CHANNELS  4U
#define ADC_MAX       4095U
#define PULSE_MIN     1000U
#define PULSE_MAX     2000U
#define LOOP_MS       50U

static uint16_t pot_to_pulse_us(uint16_t raw)
{
    uint32_t span = PULSE_MAX - PULSE_MIN;
    return (uint16_t)(PULSE_MIN + ((uint32_t)raw * span) / ADC_MAX);
}

int main(void)
{
    mcg_init_120mhz();
    systick_init(120000000UL);
    uart_init(115200U);

    uart_puts("TamgaOS K64F — 4-Channel Pot-Controlled PWM Test\r\n");
    uart_puts("[PWM4] Channels: 0=PTC1 1=PTC5 2=PTC8 3=PTC9\r\n");
    uart_puts("[PWM4] Single pot (A0) drives all 4 channels identically.\r\n\r\n");

    pwm_init();
    adc_init();

    if (adc_calibration_failed()) {
        uart_puts("[ADC] WARNING: calibration failed\r\n\r\n");
    }

    for (;;) {
        uint16_t raw   = adc_read_se12();
        uint16_t pulse = pot_to_pulse_us(raw);

        for (uint8_t ch = 0U; ch < NUM_CHANNELS; ch++) {
            pwm_set_pulse_us_ch(ch, pulse);
        }

        uart_printf("pot_raw=%u pulse=%uus (applied to all 4 channels)\r\n",
                    (unsigned int)raw, (unsigned int)pulse);

        systick_delay_ms(LOOP_MS);
    }

    return 0;
}