#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "system_clock.h"

void uart0_init(uint32_t baud);
void uart0_putc(char c);
void uart0_puts(const char *s);

#endif
