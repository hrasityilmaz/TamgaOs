/*
 * kernel/core/hardfault.c
 * TamgaOS - Generic HardFault handler (board-agnostic)
 * uart from drivers folder if not error will throw !!!
 */

#include <stdint.h>

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
        "b hardfault_c      \n");
}

void hardfault_c(uint32_t *sp)
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

    hf_puts("\r\n\r\n*** HARDFAULT ***\r\n");
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

    hf_puts("*** HALTED ***\r\n");

    __asm volatile("bkpt #0");

    while (1) {
    }
}

/*
#include <stdint.h>

#define SCB_CFSR   (*(volatile uint32_t *)0xE000ED28U)
#define SCB_HFSR   (*(volatile uint32_t *)0xE000ED2CU)
#define SCB_DFSR   (*(volatile uint32_t *)0xE000ED30U)
#define SCB_MMFAR  (*(volatile uint32_t *)0xE000ED34U)
#define SCB_BFAR   (*(volatile uint32_t *)0xE000ED38U)
#define SCB_AFSR   (*(volatile uint32_t *)0xE000ED3CU)

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

__attribute__((naked))
void HardFault_Handler(void)
{
    __asm volatile(
        "tst lr,#4          \n"
        "ite eq             \n"
        "mrseq r0,msp       \n"
        "mrsne r0,psp       \n"
        "b hardfault_c      \n");
}

void hardfault_c(uint32_t *sp)
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

    __asm volatile("bkpt #0");

    while (1) {
    }
}
*/