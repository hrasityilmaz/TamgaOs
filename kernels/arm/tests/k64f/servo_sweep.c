#include "mcg.h"
#include "systick.h"
#include "uart.h"
#include "servo.h"

int main(void)
{
    mcg_init_120mhz();
    systick_init(120000000UL);
    uart_init(115200U);

    uart_puts("TamgaOS K64F @ 120MHz\r\n");
    uart_puts("Servo Sweep Test — 4 channels (PTC1/PTC5/PTC8/PTC9)\r\n");
    uart_puts("Continuous 0 <-> 180 sweep, forever, all channels\r\n\r\n");

    servo_init();

    for (;;) {
        servo_sweep_step(0U);   /* Motor 1 — PTC1 */
        servo_sweep_step(1U);   /* Motor 2 — PTC5 */
        servo_sweep_step(2U);   /* Motor 3 — PTC8 */
        servo_sweep_step(3U);   /* Motor 4 — PTC9 */

        systick_delay_ms(20U);
    }

    return 0;
}