#include "mcg.h"
#include "mmio_deviation.h"
#include "mutex.h"
#include "pit.h"
#include "scheduler.h"
#include "semaphore.h"
#include "task.h"
#include "uart.h"

#define LED_RED_PIN (1UL << 22U)
#define LED_BLUE_PIN (1UL << 21U)

static mutex_t g_test_mutex;
static sem_t g_sem_red_to_blue;
static sem_t g_sem_blue_to_red;

// static void disable_wdog(void) {
//   WDOG->UNLOCK = WDOG_UNLOCK_KEY1;
//   WDOG->UNLOCK = WDOG_UNLOCK_KEY2;
//   WDOG->STCTRLH = WDOG_STCTRLH_DISABLE;
// }

// static void print_hex(const char *label, uint32_t val) {
//   char buf[12];
//   buf[0] = '0';
//   buf[1] = 'x';
//   buf[10] = '\r';
//   buf[11] = '\n';
//   for (int i = 9; i >= 2; i--) {
//     uint8_t last = val & 0xFU; // 0xFF ?
//     buf[i] = (last < 10U) ? ('0' + last) : ('A' + last - 10U);
//     val >>= 4;
//   }
//   uart0_puts(label);
//   uart0_puts(": ");
//   for (int i = 0; i < 12; i++)
//     uart0_putc(buf[i]);
// }

// static void disable_wdog(void) {
//   WDOG->UNLOCK = WDOG_UNLOCK_KEY1;
//   WDOG->UNLOCK = WDOG_UNLOCK_KEY2;
//   WDOG->STCTRLH = WDOG_STCTRLH_DISABLE;
// }
//

static void board_init(void) {
  // red
  SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
  PORTB->PCR[22] = PORT_PCR_MUX(1U);
  GPIOB->PDDR |= LED_RED_PIN;
  GPIOB->PSOR = LED_RED_PIN;
  // blue
  PORTB->PCR[21] = PORT_PCR_MUX(1U);
  GPIOB->PDDR |= LED_BLUE_PIN;
  GPIOB->PSOR = LED_BLUE_PIN;
}

static void task_led(void) {
  while (1) {
    // GPIOB->PCOR = LED_RED_PIN;
    // sched_delay_ms(500U);
    // GPIOB->PSOR = LED_RED_PIN;
    // sched_delay_ms(500U);
    GPIOB->PTOR |= LED_RED_PIN;
    sched_delay_ms(50U);
  }
}

// static void task_led_red(void) {
//   while (1) {
//     GPIOB->PCOR = LED_RED_PIN;
//     sched_delay_ms(500U);
//     GPIOB->PSOR = LED_RED_PIN;
//     sem_give(&g_sem_red_to_blue);
//     sem_take(&g_sem_blue_to_red);
//   }
// }

static void task_led_red(void) {
  while (1) {
    GPIOB->PCOR = LED_RED_PIN;
    sched_delay_ms(500U);
    GPIOB->PSOR = LED_RED_PIN;
    uart0_puts("[RED] before give\r\n");
    sem_give(&g_sem_red_to_blue);
    uart0_puts("[RED] after give, before take\r\n");
    sem_take(&g_sem_blue_to_red);
    uart0_puts("[RED] after take\r\n");
  }
}

static void task_led_blue(void) {
  while (1) {
    sem_take(&g_sem_red_to_blue);
    uart0_puts("[BLUE] got sem\r\n");
    GPIOB->PCOR = LED_BLUE_PIN;
    sched_delay_ms(500U);
    GPIOB->PSOR = LED_BLUE_PIN;
    sem_give(&g_sem_blue_to_red);
    uart0_puts("[BLUE] gave sem\r\n");
  }
}

static void task_uart(void) {
  while (1) {
    mutex_lock(&g_test_mutex);
    uart0_puts("[LOW] locked\r\n");
    sched_delay_ms(500U);
    uart0_puts("[LOW] unlock\r\n");
    mutex_unlock(&g_test_mutex);
    sched_delay_ms(100U);
  }
}

static void high_task_uart(void) {
  while (1) {
    mutex_lock(&g_test_mutex);
    uart0_puts("[HIGH] locked\r\n");
    sched_delay_ms(300U);
    uart0_puts("[HIGH] unlock\r\n");
    mutex_unlock(&g_test_mutex);
    sched_delay_ms(100U);
  }
}

int main(void) {
  // disable_wdog();
  mcg_init_120mhz();
  uart0_init(115200U);
  uart0_puts("TamgaOS booting...\r\n");
  board_init();

  // uint32_t pc, lr, sp, xpsr;
  //__asm volatile("mov %0, pc" : "=r"(pc));
  //__asm volatile("mov %0, lr" : "=r"(lr));
  //__asm volatile("mov %0, sp" : "=r"(sp));
  //__asm volatile("mrs %0, xpsr" : "=r"(xpsr));

  // print_hex("PC  ", pc);
  // print_hex("LR  ", lr);
  // print_hex("SP  ", sp);
  // print_hex("xPSR", xpsr);

  sched_init();
  mutex_init(&g_test_mutex);
  sem_init(&g_sem_red_to_blue, 0, 1);
  sem_init(&g_sem_blue_to_red, 0, 1);
  sched_task_create(task_led_red, TASK_PRIORITY_NORMAL);
  sched_task_create(task_led_blue, TASK_PRIORITY_NORMAL);
  // sched_task_create(task_led, TASK_PRIORITY_NORMAL);
  sched_task_create(high_task_uart, TASK_PRIORITY_HIGH);
  sched_task_create(task_uart, TASK_PRIORITY_LOW);

  pit_init(1000U);
  pit_sched_enable();

  uart0_puts("Starting scheduler...\r\n");
  sched_start();
  return 0;
}
