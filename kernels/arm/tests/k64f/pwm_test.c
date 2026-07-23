#include "mcg.h"
#include "systick.h"
#include "uart.h"
#include "pwm.h"

int main(void)
{
    mcg_init_120mhz();
    systick_init(120000000UL);
    uart_init(115200U);

    uart_puts("TamgaOS K64F @ 120MHz\r\n");
    uart_puts("PWM Test — FTM0_CH0 on PTC1 (Motor 1)\r\n\r\n");

    pwm_init();
    pwm_debug_print_regs();

    uint16_t pulse = 1000U;
    int8_t direction = 1;

    for (;;) {
        pwm_set_pulse_us_ch(0U, pulse);
        pwm_set_pulse_us_ch(1U, pulse);
        pwm_set_pulse_us_ch(2U, pulse);
        pwm_set_pulse_us_ch(3U, pulse);
        uart_printf("[PWM] Motor1 pulse=%uus\r\n", (unsigned int)pulse);

        pulse = (uint16_t)(pulse + (direction * 100));
        if (pulse >= 2000U) direction = -1;
        if (pulse <= 1000U) direction = 1;

        systick_delay_ms(200U);
    }

    return 0;
}