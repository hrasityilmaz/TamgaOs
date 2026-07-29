/*
 * tests/k64f/esc_motor_test.c — minimal single-motor ESC spin test
 * on K64F, channel 0 (Motor 1 / PTC1)
 */

#include "mcg.h"
#include "systick.h"
#include "uart.h"
#include "pwm.h"

#define MOTOR_CH        0U  
#define ARM_PULSE_US    1000U 
#define ARM_HOLD_MS     3000U 
#define RAMP_MAX_US     1300U 
#define RAMP_STEP_US    20U
#define RAMP_STEP_DELAY_MS  100U

int main(void)
{
    mcg_init_120mhz();
    systick_init(120000000UL);
    uart_init(115200U);

    uart_puts("TamgaOS K64F — ESC Single Motor Test (Channel 0 / PTC1)\r\n");
    uart_puts("!!! CONFIRM PROPELLER IS REMOVED BEFORE PROCEEDING !!!\r\n\r\n");

    pwm_init();

    uart_printf("[ESC] Arming: holding %uus for %ums\r\n",
                (unsigned int)ARM_PULSE_US, (unsigned int)ARM_HOLD_MS);
    pwm_set_pulse_us_ch(MOTOR_CH, ARM_PULSE_US);
    systick_delay_ms(ARM_HOLD_MS);
    uart_puts("[ESC] Arm hold complete. Listen for ESC beep sequence "
               "to confirm it armed before it starts ramping.\r\n\r\n");

    systick_delay_ms(1000U); 

    uart_puts("[ESC] Ramping up...\r\n");
    for (uint16_t pulse = ARM_PULSE_US; pulse <= RAMP_MAX_US; pulse += RAMP_STEP_US) {
        pwm_set_pulse_us_ch(MOTOR_CH, pulse);
        uart_printf("[ESC] pulse=%uus\r\n", (unsigned int)pulse);
        systick_delay_ms(RAMP_STEP_DELAY_MS);
    }

    uart_puts("\r\n[ESC] Holding at max test throttle for 2s...\r\n");
    systick_delay_ms(2000U);

    uart_puts("\r\n[ESC] Ramping down...\r\n");
    for (int32_t pulse = (int32_t)RAMP_MAX_US; pulse >= (int32_t)ARM_PULSE_US; pulse -= (int32_t)RAMP_STEP_US) {
        pwm_set_pulse_us_ch(MOTOR_CH, (uint16_t)pulse);
        uart_printf("[ESC] pulse=%uus\r\n", (unsigned int)pulse);
        systick_delay_ms(RAMP_STEP_DELAY_MS);
    }

    pwm_set_pulse_us_ch(MOTOR_CH, ARM_PULSE_US);
    uart_puts("\r\n[ESC] Test complete. Motor held at minimum throttle.\r\n");

    for (;;) { }
    return 0;
}