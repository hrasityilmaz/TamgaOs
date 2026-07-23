#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "servo.h"

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();

    servo_init();

    uart_puts("TamgaOS STM32H753ZI @ 480MHz\r\n");
    uart_puts("Servo Sweep Test — TIM2_CH1 on PA0 (D32)\r\n");
    uart_puts("Continuous 0 <-> 180 sweep, forever\r\n\r\n");

    for (;;) {
        servo_sweep_step();
        systick_delay_ms(20U);
    }

    return 0;
}