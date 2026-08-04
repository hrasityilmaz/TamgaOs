/*
 * Minimal newlib syscall stubs for bare-metal ARM (no OS).
 * Satisfies linker references pulled in by libc/libm (e.g. reentrancy
 * structures, stdio FILE objects) even when the application never calls
 * printf/scanf directly.
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

/* Declared in drivers/stm32h753zi/uart.h (or uart.c) */
void uart_putc(char c);

int _close(int file) {
    (void)file;
    return -1;
}

int _fstat(int file, struct stat *st) {
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) {
    (void)file;
    return 1;
}

int _lseek(int file, int ptr, int dir) {
    (void)file; (void)ptr; (void)dir;
    return 0;
}

int _read(int file, char *ptr, int len) {
    (void)file; (void)ptr; (void)len;
    return 0;
}

int _write(int file, char *ptr, int len) {
    (void)file;
    for (int i = 0; i < len; i++) {
        uart_putc(ptr[i]);
    }
    return len;
}

extern char _end;        /* set by linker script (linker.ld .heap section): start of heap */
extern char _stack_top;  /* set by linker script: top of DTCM / top of stack */

static char *heap_end = 0;

void *_sbrk(int incr) {
    char *prev_heap_end;

    if (heap_end == 0) {
        heap_end = &_end;
    }
    prev_heap_end = heap_end;

    if (heap_end + incr > &_stack_top) {
        errno = ENOMEM;
        return (void *)-1; /* heap would collide with the stack */
    }

    heap_end += incr;
    return (void *)prev_heap_end;
}

int _kill(int pid, int sig) {
    (void)pid; (void)sig;
    errno = EINVAL;
    return -1;
}

int _getpid(void) {
    return 1;
}

void _exit(int status) {
    (void)status;
    while (1) {
        /* halt */
    }
}
