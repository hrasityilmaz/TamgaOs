#include "rcc.h"
#include <stdint.h>

/*
TODO:
Change addresses and comments for misra c
*/

#define RCC_BASE 0x58024400UL

#define RCC_CR (*(volatile uint32_t *)(RCC_BASE + 0x000U))
#define RCC_CFGR (*(volatile uint32_t *)(RCC_BASE + 0x010U))

/* RCC_CR bits */
#define RCC_CR_HSION (1UL << 0U)
#define RCC_CR_HSIRDY (1UL << 2U)
#define RCC_CR_HSIDIV_MASK (3UL << 3U)
#define RCC_CR_HSIDIV_1 (0UL << 3U)

/* RCC_CFGR bits */
#define RCC_CFGR_SW_MASK (7UL << 0U)
#define RCC_CFGR_SW_HSI (0UL << 0U)
#define RCC_CFGR_SWS_MASK (7UL << 3U)
#define RCC_CFGR_SWS_HSI (0UL << 3U)

void rcc_init_hsi64(void) {
  RCC_CR |= RCC_CR_HSION;
  RCC_CR = (RCC_CR & ~RCC_CR_HSIDIV_MASK) | RCC_CR_HSIDIV_1;
  while ((RCC_CR & RCC_CR_HSIRDY) == 0U) { /* wait */
  }
}
