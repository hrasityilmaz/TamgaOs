/*
 * USART3 — Virtual COM port via ST-Link
 * TX → PD8 AF7
 * RX → PD9 AF7
 *
 * Clock tree:
 *   sysclk = 480MHz
 *   AHB    = 240MHz (HPRE=/2)
 *   APB1   = 240MHz (D2PPRE1=/1, default — RCC_D2CFGR not touched)
 *
 * BRR = APB1_clock / baud = 240000000 / 115200 = 2083
 *
 * REF: RM0433 Rev8
 *   USART3 base:  0x40004800
 *   RCC_APB1LENR: 0x58024400 + 0x0E8, USART3EN bit18
 *   GPIOD base:   0x58020C00
 *   RCC_AHB4ENR:  0x58024400 + 0x0E0, GPIODEN bit3
 */

 #include "uart.h"
 #include <stdint.h>
 #include <stdarg.h>
 #include "mutex.h"

 #define USART3_BASE     0x40004800UL
#define USART3_CR1      (*(volatile uint32_t *)(USART3_BASE + 0x000U))
#define USART3_CR2      (*(volatile uint32_t *)(USART3_BASE + 0x004U))
#define USART3_CR3      (*(volatile uint32_t *)(USART3_BASE + 0x008U))
#define USART3_BRR      (*(volatile uint32_t *)(USART3_BASE + 0x00CU))
#define USART3_ISR      (*(volatile uint32_t *)(USART3_BASE + 0x01CU))
#define USART3_RDR      (*(volatile uint32_t *)(USART3_BASE + 0x024U))
#define USART3_TDR      (*(volatile uint32_t *)(USART3_BASE + 0x028U))

#define USART_CR1_UE (1UL << 0U)
#define USART_CR1_RE (1UL << 2U)
#define USART_CR1_TE (1UL << 3U)
#define USART_ISR_RXNE  (1UL << 5U) 
#define USART_ISR_TC    (1UL << 6U) 
#define USART_ISR_TXE   (1UL << 7U) 
#define RCC_BASE        0x58024400UL
#define RCC_AHB4ENR     (*(volatile uint32_t *)(RCC_BASE + 0x0E0U))
#define RCC_APB1LENR    (*(volatile uint32_t *)(RCC_BASE + 0x0E8U))

#define RCC_AHB4ENR_GPIODEN   (1UL << 3U)   
#define RCC_APB1LENR_USART3EN (1UL << 18U) 
#define GPIOD_BASE      0x58020C00UL
#define GPIOD_MODER     (*(volatile uint32_t *)(GPIOD_BASE + 0x00U))
#define GPIOD_OSPEEDR   (*(volatile uint32_t *)(GPIOD_BASE + 0x08U))
#define GPIOD_PUPDR     (*(volatile uint32_t *)(GPIOD_BASE + 0x0CU))
#define GPIOD_AFRH      (*(volatile uint32_t *)(GPIOD_BASE + 0x24U))

static mutex_t s_uart_mutex;

void uart_init(void){
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIODEN;
    RCC_APB1LENR |= RCC_APB1LENR_USART3EN;
    // Every pin 2bit!!! PD8 8*2 PD9 9x2
    GPIOD_MODER &= ~((3UL << (8U * 2U)) | (3UL << (9U * 2U)));
    GPIOD_MODER |=   (2UL << (8U * 2U)) | (2UL << (9U * 2U));
    GPIOD_OSPEEDR |= (3UL << (8U * 2U)) | (3UL << (9U * 2U));
    GPIOD_PUPDR &= ~((3UL << (8U * 2U)) | (3UL << (9U * 2U)));

    GPIOD_AFRH &= ~((0xFUL << 0U) | (0xFUL << 4U));
    GPIOD_AFRH |=   (7UL   << 0U) | (7UL   << 4U);   /* AF7 */

    USART3_CR1 = 0U;
    USART3_CR2 = 0U;
    USART3_CR3 = 0U;

    // 240000000/115200 = 2083.3 !!
    USART3_BRR = 2083U;
    USART3_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
	mutex_init(&s_uart_mutex);
}

void uart_putc(char c)
{
    while ((USART3_ISR & USART_ISR_TXE) == 0U) {}
    USART3_TDR = (uint32_t)c;
}

static void uart_puts_raw(const char *s)
{
    while (*s)
        uart_putc(*s++);
}

void uart_puts(const char *s)
{
    mutex_lock(&s_uart_mutex);
    uart_puts_raw(s);
    mutex_unlock(&s_uart_mutex);
}

char uart_getc(void)
{
    while ((USART3_ISR & USART_ISR_RXNE) == 0U) {}
    return (char)(USART3_RDR & 0xFFU);
}

static void print_uint_padded(uint32_t n, uint8_t base, int width, int pad_zero, int upper)
{
    char buf[32];
    int  i = 31;
    buf[i] = '\0';
    if (n == 0U) {
        buf[--i] = '0';
    } else {
        while (n > 0U && i > 0) {
            uint32_t rem = n % base;
            char digit_a = upper ? 'A' : 'a';
            buf[--i] = (char)(rem < 10U ? '0' + rem : digit_a + rem - 10U);
            n /= base;
        }
    }

    int len = 31 - i;
    for (int pad = len; pad < width; pad++) {
        uart_putc(pad_zero ? '0' : ' ');
    }
    uart_puts_raw(&buf[i]);
}

static void print_uint(uint32_t n, uint8_t base)
{
    print_uint_padded(n, base, 0, 0, 0);
}

static void print_int(int32_t n)
{
    if (n < 0) {
        uart_putc('-');
        n = -n;
    }
    print_uint((uint32_t)n, 10U);
}

static void print_float(double f, int decimals)
{
    if (f < 0.0) {
        uart_putc('-');
        f = -f;
    }
    uint32_t integer = (uint32_t)f;
    print_uint(integer, 10U);
    uart_putc('.');

    double frac = f - (double)integer;
    for (int i = 0; i < decimals; i++) {
        frac *= 10.0;
        uint32_t digit = (uint32_t)frac;
        uart_putc((char)('0' + digit));
        frac -= (double)digit;
    }
}

void uart_printf(const char *fmt, ...)
{
	//mutex_lock(&s_uart_mutex);
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            uart_putc(*fmt++);
            continue;
        }
        fmt++; 
        int pad_zero = 0;
        if (*fmt == '0') {
            pad_zero = 1;
            fmt++;
        }
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }
        int decimals = 3;
        if (*fmt == '.') {
            fmt++;
            decimals = 0;
            while (*fmt >= '0' && *fmt <= '9') {
                decimals = decimals * 10 + (*fmt - '0');
                fmt++;
            }
        }

        switch (*fmt) {
            case 'd': {
                int32_t v = va_arg(args, int);
                if (v < 0) { uart_putc('-'); v = -v; }
                print_uint_padded((uint32_t)v, 10U, width, pad_zero, 0);
                break;
            }
            case 'u': print_uint_padded(va_arg(args, uint32_t), 10U, width, pad_zero, 0); break;
            case 'x': print_uint_padded(va_arg(args, uint32_t), 16U, width, pad_zero, 0); break;
            case 'X': print_uint_padded(va_arg(args, uint32_t), 16U, width, pad_zero, 1); break;
            case 's': uart_puts_raw(va_arg(args, const char *));     break;
            case 'c': uart_putc((char)va_arg(args, int));        break;
            case 'f': print_float(va_arg(args, double), decimals); break;
            case '%': uart_putc('%');                             break;
            default:  uart_putc('%'); uart_putc(*fmt);           break;
        }
        fmt++;
    }

    va_end(args);
	//mutex_unlock(&s_uart_mutex);
}

// fpu safe ...
static void uart_print_num(int n)
{
    char buf[12];
    int i = 11;
    buf[i] = '\0';
    if (n < 0) { uart_putc('-'); n = -n; }
    if (n == 0) { uart_putc('0'); return; }
    while (n > 0 && i > 0) {
        buf[--i] = '0' + (n % 10);
        n /= 10;
    }
    uart_puts(&buf[i]);
}

void uart_print_int(const char *prefix, int val)
{
    uart_puts(prefix);
    uart_print_num(val);
    uart_puts("\r\n");
}

void uart_print_2int(const char *prefix, int a, const char *sep, int b)
{
    uart_puts(prefix);
    uart_print_num(a);
    uart_putc('.');
    if (b < 0) b = -b;
    /* pad to 3 digits */
    if (b < 100) uart_putc('0');
    if (b < 10)  uart_putc('0');
    uart_print_num(b);
    uart_puts(sep);
}

void uart_print_imu(int ax, int ay, int az, int gx, int gy, int gz)
{
    uart_puts("AX:");  uart_print_num(ax);
    uart_puts(" AY:"); uart_print_num(ay);
    uart_puts(" AZ:"); uart_print_num(az);
    uart_puts(" GX:"); uart_print_num(gx);
    uart_puts(" GY:"); uart_print_num(gy);
    uart_puts(" GZ:"); uart_print_num(gz);
    uart_puts("\r\n");
}