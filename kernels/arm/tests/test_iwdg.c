/*
 * test_iwdg.c — verifies the Independent Watchdog actually resets
 * the board when starved, and that the reset cause is correctly
 * identified on the next boot.
 *
 * Test sequence:
 *   1. Boot — report any previous fault AND report whether the
 *      previous reset was caused by IWDG specifically.
 *   2. Arm IWDG with a short (1000ms) timeout.
 *   3. Kick it a few times, printing a counter, to prove kicking
 *      works and prevents reset while it's happening.
 *   4. Deliberately STOP kicking (simulate a hung task) and just
 *      spin — the board should reset itself within ~1000ms.
 *   5. On the reset that follows, this same code runs again, and
 *      step 1 this time should report "Previous reset: IWDG" —
 *      proving both the reset actually happened AND that the cause
 *      is correctly distinguishable from a normal/NRST/fault reset.
 */

#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "iwdg.h"
#include "fault_log.h"
#include <stdint.h>

static void report_boot_cause(void)
{
    fault_log_t f;
    fault_log_init();

    /* Read IWDG cause BEFORE clearing flags, and BEFORE fault_log
       potentially reports something else — these are independent
       questions ("did IWDG reset us" vs "did a fault happen") and
       both can be true after a hung task that itself resulted from
       a fault-then-halt sequence, so report both. */
    if (iwdg_reset_was_watchdog()) {
        uart_puts(">>> Previous reset cause: IWDG (watchdog timeout) <<<\r\n\r\n");
    } else {
        uart_puts(">>> Previous reset cause: NOT watchdog (NRST/POR/etc) <<<\r\n\r\n");
    }
    iwdg_clear_reset_flags();

    if (fault_log_check_and_clear(&f)) {
        uart_puts("=== Previous boot also ended in a fault ===\r\n");
        uart_printf("PC=0x%x LR=0x%x CFSR=0x%x\r\n", f.pc, f.lr, f.cfsr);
        uart_puts("============================================\r\n\r\n");
    }
}

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();

    uart_puts("TamgaOS STM32H753ZI @ 480MHz\r\n");
    uart_puts("IWDG test\r\n\r\n");

    report_boot_cause();

    uart_puts("Arming IWDG: 1000ms timeout\r\n");
    iwdg_init(1000U);

    uart_puts("Kicking 5 times (200ms apart) to prove kicking works...\r\n");
    for (uint8_t i = 1U; i <= 5U; i++) {
        systick_delay_ms(200U);
        iwdg_kick();
        uart_printf("  kick #%d\r\n", i);
    }

    uart_puts("\r\nNow STARVING the watchdog on purpose (simulated hang)...\r\n");
    uart_puts("Board should reset itself within ~1000ms. Watching...\r\n\r\n");

    /* Deliberately never call iwdg_kick() again from here on. */
    uint32_t waited_ms = 0U;
    while (1) {
        systick_delay_ms(100U);
        waited_ms += 100U;
        uart_printf("  ... %dms since last kick, still alive\r\n", waited_ms);
        /* If you see this counter pass ~1000-1500ms without a reset,
           the IWDG isn't actually resetting — check iwdg.c register
           offsets (IWDG_BASE / KR / PR / RLR) against RM0433. */
    }

    return 0;
}