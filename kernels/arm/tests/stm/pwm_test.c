#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "pwm.h"

int main(void)
{
    // System init
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();
    
    pwm_init();
    
    // Print registers
    pwm_debug_print_regs();
    
    uart_puts("\r\nPWM is running on PA0!\r\n");
    uart_puts("Frequency: 10 kHz (100 µs period)\r\n");
    uart_puts("Duty cycle: 50%%\r\n");
    uart_puts("Changing duty cycle...\r\n\r\n");

    uint16_t pulse = 1000;
    int8_t dir = 1;
    
    for (;;) {
        pwm_set_pulse_us(pulse);
        uart_printf("Pulse width: %d µs\r\n", pulse);
        
        pulse += dir * 100;
        if (pulse >= 2000) dir = -1;
        if (pulse <= 1000) dir = 1;
        systick_delay_ms(200);
    }
}