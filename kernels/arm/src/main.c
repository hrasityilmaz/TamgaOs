#include "mcg.h"
#include "mmio_deviation.h"
#include "pit.h"
#include "system_clock.h"
#include "uart.h"

#define LED_RED_PIN (1UL << 22U)

static void disable_wdog(void) {
  WDOG->UNLOCK = WDOG_UNLOCK_KEY1;
  WDOG->UNLOCK = WDOG_UNLOCK_KEY2;
  WDOG->STCTRLH = WDOG_STCTRLH_DISABLE;
}

static void board_init(void) {
  /* LED - PTB22 */
  SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
  PORTB->PCR[22] = PORT_PCR_MUX(1U);
  GPIOB->PDDR |= LED_RED_PIN;
}

/* TODO: LATER REMOBE */
static void uart_print_u32(uint32_t val) {
  char buf[11];
  uint8_t i = 0U;
  if (val == 0U) {
    uart0_putc('0');
    return;
  }
  while (val > 0U && i < 10U) {
    buf[i++] = (char)('0' + (char)(val % 10U));
    val /= 10U;
  }
  while (i > 0U) {
    uart0_putc(buf[--i]);
  }
}

int main(void) {
  // TODO: startup....
  disable_wdog(); // problem on startup fix!!!
  mcg_init_120mhz();

  uart0_init(115200U);
  uart0_puts("TamgaOS booting...\r\n");

  board_init();

  pit_init(1000U);
  uart0_puts("PIT started\r\n");

  uint32_t next_tick = pit_get_tick();
  while (1) {
    next_tick += 500U;
    while (pit_get_tick() < next_tick) {
    }

    GPIOB->PTOR = LED_RED_PIN;
    uart0_puts("tick=");
    uart_print_u32(pit_get_tick());
    uart0_puts("\r\n");
  }

  return 0; // dead maybe remove
}
