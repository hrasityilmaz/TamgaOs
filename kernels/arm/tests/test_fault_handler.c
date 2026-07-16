/*
 * test_fault_handler.c — deliberately triggers each fault type to
 * verify routing to fault_handler_c(), UART dump, and Backup SRAM
 * persistence via fault_log.
 *
 * IMPORTANT: SHCSR enable bits must be set BEFORE triggering MemManage/
 * BusFault/UsageFault, or the CPU escalates them straight to HardFault
 * regardless of the vector table entries — this would make it look
 * like MemManage_Handler/BusFault_Handler/UsageFault_Handler routing
 * is broken even if it's actually fine.
 */

#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "fault_log.h"
#include <stdint.h>

/*  Uncomment only one  */
#define FAULT_TEST_USAGEFAULT   /* divide by zero, needs DIV_0_TRP */
/* #define FAULT_TEST_BUSFAULT */    /* read from reserved/unmapped bus address */
/* #define FAULT_TEST_MEMMANAGE */   /* requires your MPU stack-guard region to be active */
/* #define FAULT_TEST_HARDFAULT */   /* forced escalation: fault while its own priority is masked */

/*  SCB  */
#define SCB_BASE   0xE000ED00UL
#define SCB_SHCSR  (*(volatile uint32_t *)(SCB_BASE + 0x24U))
#define SCB_CCR    (*(volatile uint32_t *)(SCB_BASE + 0x14U))

#define SHCSR_MEMFAULTENA  (1UL << 16U)
#define SHCSR_BUSFAULTENA  (1UL << 17U)
#define SHCSR_USGFAULTENA  (1UL << 18U)

#define CCR_DIV_0_TRP      (1UL << 4U)   /* trap on integer divide-by-zero */
#define CCR_UNALIGN_TRP    (1UL << 3U)   /* trap on unaligned access */

static void report_previous_fault(void)
{
    fault_log_t f;
    fault_log_init();

    if (fault_log_check_and_clear(&f)) {
        uart_puts("=== Previous boot ended in a fault ===\r\n");
        uart_printf("EXC_RETURN=0x%x PC=0x%x LR=0x%x xPSR=0x%x\r\n",
                    f.exc_return, f.pc, f.lr, f.xpsr);
        uart_printf("CFSR=0x%x HFSR=0x%x\r\n", f.cfsr, f.hfsr);
        if (f.mmfar_valid) uart_printf("MMFAR=0x%x\r\n", f.mmfar);
        if (f.bfar_valid)  uart_printf("BFAR=0x%x\r\n", f.bfar);
        uart_puts("=======================================\r\n\r\n");
    } else {
        uart_puts("No pending fault log (clean boot or first flash)\r\n\r\n");
    }
}

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();

    uart_puts("TamgaOS STM32H753ZI @ 480MHz\r\n");
    uart_puts("Fault handler test\r\n\r\n");

    /* Always report+clear whatever the PREVIOUS run's fault produced,
       before triggering a new one this run. */
    report_previous_fault();

    /* Enable dedicated fault handlers — without this, everything
       below escalates straight to HardFault_Handler instead of
       exercising MemManage_Handler/BusFault_Handler/UsageFault_Handler. */
    SCB_SHCSR |= (SHCSR_MEMFAULTENA | SHCSR_BUSFAULTENA | SHCSR_USGFAULTENA);
    SCB_CCR   |= (CCR_DIV_0_TRP | CCR_UNALIGN_TRP);

#if defined(FAULT_TEST_USAGEFAULT)
    uart_puts("Triggering: UsageFault (divide by zero)\r\n");
    systick_delay_ms(50U);   /* let UART finish shifting out before we fault */
    volatile int32_t a = 10;
    volatile int32_t b = 0;
    volatile int32_t c = a / b;   /* UsageFault: DIVBYZERO (CFSR bit 25) */
    (void)c;

#elif defined(FAULT_TEST_BUSFAULT)
    uart_puts("Triggering: BusFault (read from reserved bus address)\r\n");
    systick_delay_ms(50U);
    volatile uint32_t *bad = (volatile uint32_t *)0xA0000000UL; /* reserved region */
    volatile uint32_t v = *bad;   /* BusFault: precise/imprecise data bus error */
    (void)v;

#elif defined(FAULT_TEST_MEMMANAGE)
    uart_puts("Triggering: MemManage (relies on your MPU stack-guard region)\r\n");
    uart_puts("If nothing happens, your MPU guard region isn't covering this test\r\n");
    uart_puts("— tell me your MPU driver/region setup and I'll give an exact trigger.\r\n");
    systick_delay_ms(50U);
    void recurse(volatile uint32_t depth);
    recurse(0U);

#elif defined(FAULT_TEST_HARDFAULT)
    uart_puts("Triggering: forced HardFault (fault while masked)\r\n");
    systick_delay_ms(50U);
    __asm volatile ("cpsid i");   /* mask all configurable-priority exceptions */
    volatile uint32_t *bad = (volatile uint32_t *)0xA0000000UL;
    volatile uint32_t v = *bad;   /* escalates to HardFault since BusFault is masked */
    (void)v;

#else
    #error "Uncomment exactly one FAULT_TEST_* define above"
#endif

    /* Unreachable if the fault fired correctly */
    uart_puts("ERROR: fault did not trigger as expected\r\n");
    while (1) {}

    return 0;
}

#if defined(FAULT_TEST_MEMMANAGE)
void recurse(volatile uint32_t depth)
{
    volatile uint32_t pad[16];   /* burn stack fast so this doesn't take forever */
    pad[0] = depth;
    recurse(depth + 1U);
}
#endif