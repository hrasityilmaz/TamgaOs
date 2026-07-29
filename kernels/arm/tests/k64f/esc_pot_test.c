/*
 * tests/k64f/esc_manual_throttle.c — manual throttle control via
 * UART commands, for experimenting with different ESC calibration/
 * arming sequences without needing to reflash between attempts.
 *
 * Type a number (1000-2000) followed by Enter to set that pulse
 * width immediately. Useful for trying different calibration orders
 * (max-then-min, min-then-max, etc.) live, since every ESC brand
 * has its own expected sequence and this project doesn't know which
 * one you have.
 *
 * SAFETY: propeller MUST be removed before running this.
 */

#include "mcg.h"
#include "systick.h"
#include "uart.h"
#include "pwm.h"

#define MOTOR_CH  0U

static uint16_t s_current_pulse = 1000U;
static uint16_t read_pulse_from_uart(void)
{
    char buf[6];
    uint8_t len = 0U;

    for (;;) {
        char c = uart_getc(); 
        if (c == '\r' || c == '\n') {
            uart_puts("\r\n");
            break;
        }
        if (c >= '0' && c <= '9' && len < 5U) {
            buf[len++] = c;
            uart_putc(c);   /* echo */
        }
    }
    buf[len] = '\0';

    if (len == 0U) {
        return 0xFFFFU;
    }

    uint32_t val = 0U;
    for (uint8_t i = 0U; i < len; i++) {
        val = val * 10U + (uint32_t)(buf[i] - '0');
    }

    return (uint16_t)val;
}

int main(void)
{
    mcg_init_120mhz();
    systick_init(120000000UL);
    uart_init(115200U);

    uart_puts("TamgaOS K64F — Manual ESC Throttle Control\r\n");
    uart_puts("!!! CONFIRM PROPELLER IS REMOVED !!!\r\n\r\n");
    uart_puts("Type a pulse width (1000-2000) and press Enter.\r\n");
    uart_puts("Examples: 2000 (max, for cal), 1000 (min/arm), 1500 (mid)\r\n\r\n");

    pwm_init();
    pwm_set_pulse_us_ch(MOTOR_CH, s_current_pulse);
    uart_printf("[MANUAL] initial pulse=%uus\r\n\r\n", (unsigned int)s_current_pulse);

    for (;;) {
        uart_puts("> ");
        uint16_t val = read_pulse_from_uart();

        if (val == 0xFFFFU) {
            continue;
        }
        if (val < 1000U) val = 1000U;
        if (val > 2000U) val = 2000U;

        s_current_pulse = val;
        pwm_set_pulse_us_ch(MOTOR_CH, s_current_pulse);
        uart_printf("[MANUAL] pulse=%uus\r\n", (unsigned int)s_current_pulse);
    }

    return 0;
}