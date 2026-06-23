/*
 * hardfault.c
 * TamgaOS - HardFault UART debug handler
 *
 * Fault anındaki tüm bilgileri UART'a basar:
 *   PC, LR, PSR, R0-R3, R12, SP
 *   CFSR (neden fault olduğu)
 *   BFAR, MMFAR (adres hataları)
 */

#include <stdint.h>

#define UART0_S1 (*(volatile uint8_t *)0x4006A004U)
#define UART0_D (*(volatile uint8_t *)0x4006A007U)
#define UART0_TDRE (1U << 7U)
#define SCB_CFSR (*(volatile uint32_t *)0xE000ED28U) /* Combined fault status  \
                                                      */
#define SCB_HFSR (*(volatile uint32_t *)0xE000ED2CU) /* HardFault status */
#define SCB_BFAR (*(volatile uint32_t *)0xE000ED38U) /* BusFault address */
#define SCB_MMFAR                                                              \
  (*(volatile uint32_t *)0xE000ED34U) /* MemManage address     */

static void hf_putc(char c) {
  while (!(UART0_S1 & UART0_TDRE)) {
  }
  UART0_D = (uint8_t)c;
}

static void hf_puts(const char *s) {
  while (*s)
    hf_putc(*s++);
}

static void hf_print_hex(uint32_t val) {
  hf_puts("0x");
  for (int i = 28; i >= 0; i -= 4) {
    uint8_t nibble = (val >> i) & 0xFU;
    hf_putc(nibble < 10U ? ('0' + nibble) : ('A' + nibble - 10U));
  }
}

/* [0]=R0 [1]=R1 [2]=R2 [3]=R3 [4]=R12 [5]=LR [6]=PC [7]=xPSR */
void hardfault_c_handler(uint32_t *frame) {
  hf_puts("\r\n\r\n*** HARDFAULT ***\r\n");

  hf_puts("PC   = ");
  hf_print_hex(frame[6]);
  hf_puts("\r\n");
  hf_puts("LR   = ");
  hf_print_hex(frame[5]);
  hf_puts("\r\n");
  hf_puts("xPSR = ");
  hf_print_hex(frame[7]);
  hf_puts("\r\n");
  hf_puts("R0   = ");
  hf_print_hex(frame[0]);
  hf_puts("\r\n");
  hf_puts("R1   = ");
  hf_print_hex(frame[1]);
  hf_puts("\r\n");
  hf_puts("R2   = ");
  hf_print_hex(frame[2]);
  hf_puts("\r\n");
  hf_puts("R3   = ");
  hf_print_hex(frame[3]);
  hf_puts("\r\n");
  hf_puts("R12  = ");
  hf_print_hex(frame[4]);
  hf_puts("\r\n");
  hf_puts("SP   = ");
  hf_print_hex((uint32_t)frame);
  hf_puts("\r\n");

  hf_puts("CFSR = ");
  hf_print_hex(SCB_CFSR);
  hf_puts("\r\n");
  hf_puts("HFSR = ");
  hf_print_hex(SCB_HFSR);
  hf_puts("\r\n");
  hf_puts("BFAR = ");
  hf_print_hex(SCB_BFAR);
  hf_puts("\r\n");
  hf_puts("MMFAR= ");
  hf_print_hex(SCB_MMFAR);
  hf_puts("\r\n");
  uint32_t cfsr = SCB_CFSR;
  hf_puts("--- CFSR decode ---\r\n");
  if (cfsr & (1U << 25U))
    hf_puts("  DIVBYZERO\r\n");
  if (cfsr & (1U << 24U))
    hf_puts("  UNALIGNED\r\n");
  if (cfsr & (1U << 19U))
    hf_puts("  NOCP (no coprocessor)\r\n");
  if (cfsr & (1U << 18U))
    hf_puts("  INVPC\r\n");
  if (cfsr & (1U << 17U))
    hf_puts("  INVSTATE\r\n");
  if (cfsr & (1U << 16U))
    hf_puts("  UNDEFINSTR\r\n");
  if (cfsr & (1U << 15U))
    hf_puts("  BFARVALID\r\n");
  if (cfsr & (1U << 12U))
    hf_puts("  STKERR\r\n");
  if (cfsr & (1U << 11U))
    hf_puts("  UNSTKERR\r\n");
  if (cfsr & (1U << 10U))
    hf_puts("  IMPRECISERR\r\n");
  if (cfsr & (1U << 9U))
    hf_puts("  PRECISERR\r\n");
  if (cfsr & (1U << 8U))
    hf_puts("  IBUSERR\r\n");
  if (cfsr & (1U << 3U))
    hf_puts("  MUNSTKERR\r\n");
  if (cfsr & (1U << 1U))
    hf_puts("  DACCVIOL\r\n");
  if (cfsr & (1U << 0U))
    hf_puts("  IACCVIOL\r\n");

  hf_puts("*** HALTED ***\r\n");
  while (1) {
  }
}
