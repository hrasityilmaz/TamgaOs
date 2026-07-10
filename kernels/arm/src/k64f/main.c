#include "mcg.h"
#include "mmio_deviation.h"
#include "mutex.h"
#include "pit.h"
#include "scheduler.h"
#include "semaphore.h"
#include "task.h"
#include "uart.h"
#include "fpu_test.h"

#define LED_RED_PIN  (1UL << 22U)
#define LED_BLUE_PIN (1UL << 21U)

static mutex_t g_test_mutex;
static sem_t   g_sem_red_to_blue;
static sem_t   g_sem_blue_to_red;

static void board_init(void) {
  /* red */
  SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
  PORTB->PCR[22] = PORT_PCR_MUX(1U);
  GPIOB->PDDR |= LED_RED_PIN;
  GPIOB->PSOR = LED_RED_PIN;
  /* blue */
  PORTB->PCR[21] = PORT_PCR_MUX(1U);
  GPIOB->PDDR |= LED_BLUE_PIN;
  GPIOB->PSOR = LED_BLUE_PIN;
}

// Semaphore led task 
static void task_led_red(void) {
  while (1) {
    GPIOB->PCOR = LED_RED_PIN;
    sched_delay_ms(500U);
    GPIOB->PSOR = LED_RED_PIN;
    uart_puts("[RED] before give\r\n");
    sem_give(&g_sem_red_to_blue);
    uart_puts("[RED] after give, before take\r\n");
    sem_take(&g_sem_blue_to_red);
    uart_puts("[RED] after take\r\n");
  }
}

static void task_led_blue(void) {
  while (1) {
    sem_take(&g_sem_red_to_blue);
    uart_puts("[BLUE] got sem\r\n");
    GPIOB->PCOR = LED_BLUE_PIN;
    sched_delay_ms(500U);
    GPIOB->PSOR = LED_BLUE_PIN;
    sem_give(&g_sem_blue_to_red);
    uart_puts("[BLUE] gave sem\r\n");
  }
}

static void task_uart_low(void) {
  while (1) {
    mutex_lock(&g_test_mutex);
    uart_puts("[LOW] locked\r\n");
    sched_delay_ms(500U);
    uart_puts("[LOW] unlock\r\n");
    mutex_unlock(&g_test_mutex);
    sched_delay_ms(100U);
  }
}

static void task_uart_high(void) {
  while (1) {
    mutex_lock(&g_test_mutex);
    uart_puts("[HIGH] locked\r\n");
    sched_delay_ms(300U);
    uart_puts("[HIGH] unlock\r\n");
    mutex_unlock(&g_test_mutex);
    sched_delay_ms(100U);
  }
}

static void task_heartbeat(void) {
  uint32_t count = 0U;
  while (1) {
    uart_puts("[HB] tick\r\n");
    count++;
    sched_delay_ms(1000U);
    (void)count;
  }
}

static void uart_print_uint(const char *prefix, uint32_t val) {
  char buf[12];
  int i = 11;
  buf[i] = '\0';
  if (val == 0U) {
    uart_puts(prefix);
    uart_putc('0');
    uart_puts("\r\n");
    return;
  }
  while (val > 0U && i > 0) {
    buf[--i] = (char)('0' + (val % 10U));
    val /= 10U;
  }
  uart_puts(prefix);
  uart_puts(&buf[i]);
  uart_puts("\r\n");
}

/* Stack canary watchdog
 * (kernel/core/scheduler.c: sched_check_stack_canaries) */
static void task_watchdog(void) {
  while (1) {
    uint8_t bad = sched_check_stack_canaries();
    if (bad != 0xFFU) {
      uart_print_uint("!!! STACK OVERFLOW detected on task_idx=", (uint32_t)bad);
      while (1) {
      }
    }
    sched_delay_ms(100U);
  }
}

int main(void) {
  mcg_init_120mhz();
  uart_init(115200U);
  uart_puts("TamgaOS booting (stress test)...\r\n");
  board_init();

  sched_init();
  mutex_init(&g_test_mutex);
  sem_init(&g_sem_red_to_blue, 0, 1);
  sem_init(&g_sem_blue_to_red, 0, 1);

  sched_task_create(task_led_red,   TASK_PRIORITY_NORMAL);
  sched_task_create(task_led_blue,  TASK_PRIORITY_NORMAL);
  sched_task_create(task_uart_high, TASK_PRIORITY_HIGH);
  sched_task_create(task_uart_low,  TASK_PRIORITY_LOW);
  sched_task_create(task_heartbeat, TASK_PRIORITY_NORMAL);
  sched_task_create(task_watchdog,  TASK_PRIORITY_LOW);
  sched_task_create(task_fpu_test_a, TASK_PRIORITY_NORMAL);
  sched_task_create(task_fpu_test_b, TASK_PRIORITY_NORMAL);


  pit_init(1000U);
  pit_sched_enable();  // Later will fix seperate enable call !! (for m7 port dont need to call)

  uart_puts("Starting scheduler...\r\n");
  sched_start();
  return 0;
}