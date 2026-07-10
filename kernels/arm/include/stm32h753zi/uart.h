#ifndef UART_H
#define UART_H

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
char uart_getc(void);
//void uart_printf(const char *fmt, ...);
void uart_print_int(const char *prefix, int val);
void uart_print_2int(const char *prefix, int a, const char *sep, int b);
void uart_print_imu(int ax, int ay, int az, int gx, int gy, int gz);

#endif