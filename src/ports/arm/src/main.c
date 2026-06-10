#include <stdint.h>

// Here for now ı will use NXP-K64F ı have this card for now for ARM-CORTEX M4
// Later STMs also ı will add ı hope (I dont have money :) )
//

#define SIM_SCGC5 (*(volatile uint32_t *)0x40048038)
#define SIM_SCGC5_PORTB (1U << 10) // set portB clock bit

#define PORTB_PCR22 (*(volatile uint32_t *)0x4004A058)
#define PORT_PCR_MUX_GPIO (1U << 8);

#define GPIOB_PDDR (*(volatile uint32_t *)0x400FF054)
#define GPIOB_PTOR (*(volatile uint32_t *)0x400FF04C)
#define LED_RED_PIN (1U << 22)

static void delay(volatile uint32_t n) {
  while (n--)
    ;
}

int main(void) {
  SIM_SCGC5 |= SIM_SCGC5_PORTB;

  PORTB_PCR22 = PORT_PCR_MUX_GPIO;

  GPIOB_PDDR |= LED_RED_PIN;

  while (1) {
    GPIOB_PTOR = LED_RED_PIN;
    delay(500000);
  }

  return 0;
}
