/*
 * fpu_test.c
 * TamgaOS - FPU context switch test
 */

#include "scheduler.h"
#include "uart.h"
#include <stdint.h>

extern void uart_putc(char c);

static void fpu_puts(const char *s) {
  while (*s)
    uart_putc(*s++);
}

static void fpu_print_uint(uint32_t v) {
  char buf[12];
  int i = 11;
  buf[i] = '\0';
  if (v == 0U) { uart_putc('0'); return; }
  while (v > 0U && i > 0) {
    buf[--i] = (char)('0' + (v % 10U));
    v /= 10U;
  }
  fpu_puts(&buf[i]);
}

static void fpu_load_pattern(const float *src) {
  __asm volatile (
      "vldmia %0, {s16-s31}" :: "r"(src) : "memory"
  );
}

static void fpu_store_pattern(float *dst) {
  __asm volatile (
      "vstmia %0, {s16-s31}" :: "r"(dst) : "memory"
  );
}

static uint32_t s_fail_count_a;
static uint32_t s_fail_count_b;

void task_fpu_test_a(void) {
  float pattern[16];
  float readback[16];
  uint32_t iteration = 0U;

  for (int i = 0; i < 16; i++) {
    pattern[i] = 100.0f + (float)i;
  }

  while (1) {
    fpu_load_pattern(pattern);
    volatile float dummy = pattern[0] * 1.0001f;
    (void)dummy;
    sched_delay_ms(37U);

    fpu_store_pattern(readback);

    uint8_t ok = 1U;
    for (int i = 0; i < 16; i++) {
      if (readback[i] != pattern[i]) {
        ok = 0U;
        break;
      }
    }

    iteration++;
    if (!ok) {
      s_fail_count_a++;
      fpu_puts("[FPU-A] MISMATCH! iteration=");
      fpu_print_uint(iteration);
      fpu_puts(" fail_count=");
      fpu_print_uint(s_fail_count_a);
      fpu_puts("\r\n");
    } else if ((iteration % 50U) == 0U) {
      fpu_puts("[FPU-A] OK, iteration=");
      fpu_print_uint(iteration);
      fpu_puts("\r\n");
    }
  }
}

void task_fpu_test_b(void) {
  float pattern[16];
  float readback[16];
  uint32_t iteration = 0U;

  for (int i = 0; i < 16; i++) {
    pattern[i] = 200.0f + (float)i;
  }

  while (1) {
    fpu_load_pattern(pattern);

    volatile float dummy = pattern[0] * 1.0001f;
    (void)dummy;

    sched_delay_ms(53U);

    fpu_store_pattern(readback);

    uint8_t ok = 1U;
    for (int i = 0; i < 16; i++) {
      if (readback[i] != pattern[i]) {
        ok = 0U;
        break;
      }
    }

    iteration++;
    if (!ok) {
      s_fail_count_b++;
      fpu_puts("[FPU-B] MISMATCH! iteration=");
      fpu_print_uint(iteration);
      fpu_puts(" fail_count=");
      fpu_print_uint(s_fail_count_b);
      fpu_puts("\r\n");
    } else if ((iteration % 50U) == 0U) {
      fpu_puts("[FPU-B] OK, iteration=");
      fpu_print_uint(iteration);
      fpu_puts("\r\n");
    }
  }
}