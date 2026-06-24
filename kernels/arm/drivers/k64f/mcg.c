/*
 *   CORE     = 120MHz  (OUTDIV1 = /1)
 *   BUS      =  60MHz  (OUTDIV2 = /2)
 *   FLEXBUS  =  40MHz  (OUTDIV3 = /3)
 *   FLASH    =  24MHz  (OUTDIV4 = /5)
 *
 * PLL: EXTAL(50MHz) / (PRDIV+1) * (VDIV+24)
 *      50MHz / 15 * 36 = 120MHz
 *
 * K64F RM Chapter 25 (MCG), Chapter 27 (OSC)
 */

#include "mcg.h"
#include "mmio_deviation.h"

static void mcg_delay(volatile uint32_t n) {
  while (n-- != 0U) {
    __asm volatile("nop");
  }
}

void mcg_init_120mhz(void) {
  SIM->CLKDIV1 = SIM_CLKDIV1_OUTDIV1(0U) | /* Core  /1 = 120MHz */
                 SIM_CLKDIV1_OUTDIV2(1U) | /* Bus   /2 =  60MHz */
                 SIM_CLKDIV1_OUTDIV3(2U) | /* Flex  /3 =  40MHz */
                 SIM_CLKDIV1_OUTDIV4(4U);  /* Flash /5 =  24MHz */

  MCG->C2 = MCG_C2_RANGE0(2U) | MCG_C2_HGO0_MASK | MCG_C2_EREFS0_MASK;
  *(volatile uint8_t *)0x40065000U = 0x02U;
  mcg_delay(10000U);
  MCG->C1 = MCG_C1_IRCLKEN_MASK;
  *(volatile uint8_t *)0x40064008U = 0x01U;
  MCG->C5 = 14U;
  MCG->C6 = 12U;
  MCG->C1 =
      (uint8_t)((MCG->C1 & (uint8_t)(~MCG_C1_CLKS_MASK)) | MCG_C1_CLKS(2U));

  while (((MCG->S >> MCG_S_CLKST_SHIFT) & 0x3U) != 2U) {
  }
  MCG->C6 |= MCG_C6_PLLS_MASK;
  while ((MCG->S & MCG_S_PLLST_MASK) == 0U) {
    /*WAIT*/
  }
  while ((MCG->S & MCG_S_LOCK0_MASK) == 0U) {
    /*WAIT*/
  }
  MCG->C1 = (uint8_t)(MCG->C1 & (uint8_t)(~MCG_C1_CLKS_MASK));
  while (((MCG->S >> MCG_S_CLKST_SHIFT) & 0x3U) != 3U) {
  }
  SIM->SOPT2 = (SIM->SOPT2 & ~SIM_SOPT2_PLLFLLSEL_MASK) |
               SIM_SOPT2_PLLFLLSEL(SIM_SOPT2_PLLFLLSEL_MCGPLLCLK);
}
