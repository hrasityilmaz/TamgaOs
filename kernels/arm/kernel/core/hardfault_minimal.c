/* GEÇİCİ: K64F için minimal fault handler, fault_log.h olmadan */
#include <stdint.h>

extern void uart_putc(char c);
static void hf_puts(const char *s) { while (*s) uart_putc(*s++); }

void HardFault_Handler(void) { hf_puts("\r\n*** HARDFAULT ***\r\n"); while(1) {} }
void MemManage_Handler(void) { hf_puts("\r\n*** MEMFAULT ***\r\n"); while(1) {} }
void BusFault_Handler(void)  { hf_puts("\r\n*** BUSFAULT ***\r\n"); while(1) {} }
void UsageFault_Handler(void){ hf_puts("\r\n*** USAGEFAULT ***\r\n"); while(1) {} }