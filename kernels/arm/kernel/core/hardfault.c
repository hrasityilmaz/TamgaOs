/*
 * kernel/core/hardfault.c
 * TamgaOS - Generic fault handler (board-agnostic)
 * uart from drivers folder if not error will throw !!!
 *
 * Handles HardFault, MemManage, BusFault, and UsageFault — all four
 * share the same entry pattern (naked stub reads MSP/PSP based on
 * EXC_RETURN bit 2) and the same hardfault_c() body, since it reads
 * CFSR/HFSR/etc directly from the SCB and doesn't need to know which
 * vector fired; the CFSR bit decode already reports the right bits
 * for whichever fault actually happened.
 */

#include <stdint.h>
#include "fault_log.h"

#define SCB_CFSR   (*(volatile uint32_t *)0xE000ED28U)
#define SCB_HFSR   (*(volatile uint32_t *)0xE000ED2CU)
#define SCB_DFSR   (*(volatile uint32_t *)0xE000ED30U)
#define SCB_MMFAR  (*(volatile uint32_t *)0xE000ED34U)
#define SCB_BFAR   (*(volatile uint32_t *)0xE000ED38U)
#define SCB_AFSR   (*(volatile uint32_t *)0xE000ED3CU)

// GDB print hf_cfsr, print hf_pc etc
volatile uint32_t hf_r0;
volatile uint32_t hf_r1;
volatile uint32_t hf_r2;
volatile uint32_t hf_r3;
volatile uint32_t hf_r12;
volatile uint32_t hf_lr;
volatile uint32_t hf_pc;
volatile uint32_t hf_psr;

volatile uint32_t hf_cfsr;
volatile uint32_t hf_hfsr;
volatile uint32_t hf_dfsr;
volatile uint32_t hf_mmfar;
volatile uint32_t hf_bfar;
volatile uint32_t hf_afsr;

// uart.c from drivers
extern void uart_putc(char c);

static void hf_puts(const char *s) {
  while (*s)
    uart_putc(*s++);
}

static void hf_print_hex(uint32_t val) {
  hf_puts("0x");
  for (int i = 28; i >= 0; i -= 4) {
    uint8_t nibble = (uint8_t)((val >> i) & 0xFU);
    uart_putc(nibble < 10U ? (char)('0' + nibble) : (char)('A' + nibble - 10U));
  }
}

static void hf_print_reg(const char *name, uint32_t val) {
  hf_puts(name);
  hf_print_hex(val);
  hf_puts("\r\n");
}

__attribute__((naked))
void HardFault_Handler(void)
{
    __asm volatile(
        "tst lr,#4          \n"
        "ite eq             \n"
        "mrseq r0,msp       \n"
        "mrsne r0,psp       \n"
        "mov   r1,lr        \n"
        "b hardfault_c      \n");
}

__attribute__((naked))
void MemManage_Handler(void)
{
    __asm volatile(
        "tst lr,#4          \n"
        "ite eq             \n"
        "mrseq r0,msp       \n"
        "mrsne r0,psp       \n"
        "mov   r1,lr        \n"
        "b hardfault_c      \n");
}

__attribute__((naked))
void BusFault_Handler(void)
{
    __asm volatile(
        "tst lr,#4          \n"
        "ite eq             \n"
        "mrseq r0,msp       \n"
        "mrsne r0,psp       \n"
        "mov   r1,lr        \n"
        "b hardfault_c      \n");
}

__attribute__((naked))
void UsageFault_Handler(void)
{
    __asm volatile(
        "tst lr,#4          \n"
        "ite eq             \n"
        "mrseq r0,msp       \n"
        "mrsne r0,psp       \n"
        "mov   r1,lr        \n"
        "b hardfault_c      \n");
}

void hardfault_c(uint32_t *sp, uint32_t exc_return)
{
    hf_r0  = sp[0];
    hf_r1  = sp[1];
    hf_r2  = sp[2];
    hf_r3  = sp[3];
    hf_r12 = sp[4];
    hf_lr  = sp[5];
    hf_pc  = sp[6];
    hf_psr = sp[7];

    hf_cfsr  = SCB_CFSR;
    hf_hfsr  = SCB_HFSR;
    hf_dfsr  = SCB_DFSR;
    hf_mmfar = SCB_MMFAR;
    hf_bfar  = SCB_BFAR;
    hf_afsr  = SCB_AFSR;

    hf_puts("\r\n\r\n*** FAULT ***\r\n");
    hf_puts((exc_return & 0x4U) ? "Stack = PSP\r\n" : "Stack = MSP\r\n");
    hf_print_reg("PC   = ", hf_pc);
    hf_print_reg("LR   = ", hf_lr);
    hf_print_reg("xPSR = ", hf_psr);
    hf_print_reg("R0   = ", hf_r0);
    hf_print_reg("R1   = ", hf_r1);
    hf_print_reg("R2   = ", hf_r2);
    hf_print_reg("R3   = ", hf_r3);
    hf_print_reg("R12  = ", hf_r12);
    hf_print_reg("SP   = ", (uint32_t)sp);
    hf_print_reg("CFSR = ", hf_cfsr);
    hf_print_reg("HFSR = ", hf_hfsr);
    hf_print_reg("DFSR = ", hf_dfsr);
    hf_print_reg("BFAR = ", hf_bfar);
    hf_print_reg("MMFAR= ", hf_mmfar);
    hf_print_reg("AFSR = ", hf_afsr);

    hf_puts("--- CFSR decode ---\r\n");
    if (hf_cfsr & (1UL << 25U)) hf_puts("  DIVBYZERO\r\n");
    if (hf_cfsr & (1UL << 24U)) hf_puts("  UNALIGNED\r\n");
    if (hf_cfsr & (1UL << 19U)) hf_puts("  NOCP (no coprocessor)\r\n");
    if (hf_cfsr & (1UL << 18U)) hf_puts("  INVPC\r\n");
    if (hf_cfsr & (1UL << 17U)) hf_puts("  INVSTATE\r\n");
    if (hf_cfsr & (1UL << 16U)) hf_puts("  UNDEFINSTR\r\n");
    if (hf_cfsr & (1UL << 15U)) hf_puts("  BFARVALID\r\n");
    if (hf_cfsr & (1UL << 12U)) hf_puts("  STKERR\r\n");
    if (hf_cfsr & (1UL << 11U)) hf_puts("  UNSTKERR\r\n");
    if (hf_cfsr & (1UL << 10U)) hf_puts("  IMPRECISERR\r\n");
    if (hf_cfsr & (1UL <<  9U)) hf_puts("  PRECISERR\r\n");
    if (hf_cfsr & (1UL <<  8U)) hf_puts("  IBUSERR\r\n");
    if (hf_cfsr & (1UL <<  7U)) hf_puts("  MMARVALID\r\n");
    if (hf_cfsr & (1UL <<  4U)) hf_puts("  MSTKERR\r\n");
    if (hf_cfsr & (1UL <<  3U)) hf_puts("  MUNSTKERR\r\n");
    if (hf_cfsr & (1UL <<  1U)) hf_puts("  DACCVIOL\r\n");
    if (hf_cfsr & (1UL <<  0U)) hf_puts("  IACCVIOL\r\n");

    fault_log_t entry;
    entry.exc_return  = exc_return;
    entry.pc          = hf_pc;
    entry.lr          = hf_lr;
    entry.xpsr        = hf_psr;
    entry.r0          = hf_r0;
    entry.r1          = hf_r1;
    entry.r2          = hf_r2;
    entry.r3          = hf_r3;
    entry.r12         = hf_r12;
    entry.cfsr        = hf_cfsr;
    entry.hfsr        = hf_hfsr;
    entry.mmfar       = hf_mmfar;
    entry.bfar        = hf_bfar;
    entry.mmfar_valid = (hf_cfsr & (1UL << 7U))  ? 1U : 0U; 
    entry.bfar_valid  = (hf_cfsr & (1UL << 15U)) ? 1U : 0U;
    fault_log_write(&entry);

    if (fault_log_peek_magic() == FAULT_LOG_MAGIC) {
        hf_puts("Backup SRAM write: OK\r\n");
    } else {
        hf_puts("Backup SRAM write: FAILED (check PWR_CR1.DBP / RCC_AHB4ENR.BKPRAMEN)\r\n");
    }

    hf_puts("*** HALTED ***\r\n");
    SCB_CFSR = hf_cfsr;

    /*
     * NOTE: bkpt #0 was here previously. Removed deliberately — on
     * real hardware with no debugger attached, executing BKPT is not
     * a no-op: the CPU treats it as an unhandled debug event and
     * faults again immediately (observed as a second, nested
     * "*** FAULT ***" dump right after this one, with no reboot in
     * between — HFSR.FORCED set on the second dump confirms this).
     * If you want breakpoint-on-fault behavior for an actual GDB
     * session, gate it behind a build flag so it isn't hit on
     * unattended/field runs:
     *
     *   #ifdef FAULT_HANDLER_DEBUGGER_ATTACHED
     *   __asm volatile("bkpt #0");
     *   #endif
     */

    while (1) {
    }
}