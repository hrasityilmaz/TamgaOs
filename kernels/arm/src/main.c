#include "mcg.h"
#include "mmio_deviation.h"
#include "pit.h"
#include "scheduler.h"
#include "task.h"
#include "uart.h"

#define LED_RED_PIN (1UL << 22U)

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
  SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
  PORTB->PCR[22] = PORT_PCR_MUX(1U);
  GPIOB->PDDR |= LED_RED_PIN;
  GPIOB->PSOR = LED_RED_PIN;
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

static void task_uart(void) {
  while (1) {
    uart0_puts("TamgaOS running [1Sec]\r\n");
    sched_delay_ms(1000U);
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
  sched_task_create(task_led, TASK_PRIORITY_NORMAL);
  sched_task_create(task_uart, TASK_PRIORITY_HIGH);

  pit_init(1000U);
  pit_sched_enable();

  uart0_puts("Starting scheduler...\r\n");
  sched_start();
  return 0;
}
