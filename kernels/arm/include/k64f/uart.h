#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "system_clock.h"

void uart_init(uint32_t baud);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_printf(const char *fmt, ...);

#endif
