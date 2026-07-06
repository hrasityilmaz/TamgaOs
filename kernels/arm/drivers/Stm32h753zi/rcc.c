/*
 * rcc.c — STM32H753ZI RCC init (bare-metal)
 *
 * rcc_init_hsi64()   — HSI 64MHz
 * rcc_init_pll_480() — HSE 8MHz → PLL1 → 480MHz
 *
 * PLL1 config (verified via IDE debug):
 *   HSE = 8MHz (Nucleo-144 onboard)
 *   DIVM1 = 2  → Fref = 4MHz
 *   DIVN1 = 239 → VCO = 4 × 240 = 960MHz (wide range ✓)
 *   DIVP1 = 1  (register value, P=2) → sysclk = 480MHz
 *   PLL1RGE = 10 (4-8MHz input range)
 *   PLL1VCOSEL = 0 (wide VCO: 192-960MHz)
 *
 * Clock tree:
 *   CPU  (D1CPRE=/1) = 480MHz
 *   AHB  (HPRE=/2)   = 240MHz
 *   APB3 (D1PPRE=/2) = 120MHz
 *
 * VOS1 sequence:
 *   SYSCFG clock enable → PWR_D3CR VOS=11 → wait VOSRDY
 *
 * Flash: LATENCY=4, WRHIGHFREQ=2 (AHB=240MHz, VOS1)
 *
 * SysTick: CLKSOURCE=0 (AHB/8) — STM32H7 specific
 *   systick_init(480000000U) → reload = 480000000/8/1000-1 = 59999
 *
 * REF: RM0433
 */

#include "rcc.h"
#include <stdint.h>

/* ── RCC ── */
#define RCC_BASE 0x58024400UL
#define RCC_CR (*(volatile uint32_t *)(RCC_BASE + 0x000U))
#define RCC_CFGR (*(volatile uint32_t *)(RCC_BASE + 0x010U))
#define RCC_D1CFGR (*(volatile uint32_t *)(RCC_BASE + 0x018U))
#define RCC_PLLCKSELR (*(volatile uint32_t *)(RCC_BASE + 0x028U))
#define RCC_PLLCFGR (*(volatile uint32_t *)(RCC_BASE + 0x02CU))
#define RCC_PLL1DIVR (*(volatile uint32_t *)(RCC_BASE + 0x030U))
#define RCC_PLL1FRACR (*(volatile uint32_t *)(RCC_BASE + 0x034U))
#define RCC_APB4ENR (*(volatile uint32_t *)(RCC_BASE + 0x0F4U))

/* RCC_CR bits */
#define RCC_CR_HSION (1UL << 0U)
#define RCC_CR_HSIRDY (1UL << 2U)
#define RCC_CR_HSEON (1UL << 16U)
#define RCC_CR_HSERDY (1UL << 17U)
#define RCC_CR_PLL1ON (1UL << 24U)
#define RCC_CR_PLL1RDY (1UL << 25U)

#define RCC_CFGR_SW_HSI (0UL << 0U)
#define RCC_CFGR_SW_PLL1 (3UL << 0U)
#define RCC_CFGR_SW_MASK (7UL << 0U)
#define RCC_CFGR_SWS_MASK (7UL << 3U)
#define RCC_CFGR_SWS_HSI (0UL << 3U)
#define RCC_CFGR_SWS_PLL1 (3UL << 3U)

#define RCC_D1CFGR_HPRE_DIV2 (8UL << 0U)
#define RCC_D1CFGR_D1PPRE_DIV2 (4UL << 4U)

#define RCC_PLLCKSELR_PLLSRC_HSE (2UL << 0U)
#define RCC_PLLCKSELR_DIVM1(x) ((uint32_t)(x) << 4U)

#define RCC_PLLCFGR_PLL1VCOSEL (1UL << 1U)
#define RCC_PLLCFGR_PLL1RGE(x) ((uint32_t)(x) << 2U)
#define RCC_PLLCFGR_DIVP1EN (1UL << 16U)

#define RCC_APB4ENR_SYSCFGEN (1UL << 1U)

#define PWR_BASE 0x58024800UL
#define PWR_D3CR (*(volatile uint32_t *)(PWR_BASE + 0x018U))
#define PWR_D3CR_VOS1 (3UL << 14U)
#define PWR_D3CR_VOSRDY (1UL << 13U)

#define FLASH_ACR (*(volatile uint32_t *)0x52002000U)

static void delay(volatile uint32_t n) {
  while (n--)
    __asm volatile("nop");
}

void rcc_init_hsi64(void) {
  RCC_CR |= RCC_CR_HSION;
  while ((RCC_CR & RCC_CR_HSIRDY) == 0U) {
  }
  RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_HSI;
  while ((RCC_CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_HSI) {
  }
}

/* ─────────────────────────────────────────────────────────────
 * rcc_init_pll_480 — HSE 8MHz → PLL1 → 480MHz
 * MUST CONTROL !!!
 * ───────────────────────────────────────────────────────────── */
void rcc_init_pll_480(void) {
  /* 1. Switch to HSI, disable PLL1 */
  RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_HSI;
  while ((RCC_CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_HSI) {
  }
  RCC_CR &= ~RCC_CR_PLL1ON;
  while ((RCC_CR & RCC_CR_PLL1RDY) != 0U) {
  }
  RCC_PLLCKSELR = 0U;
  RCC_PLLCFGR = 0U;
  RCC_PLL1DIVR = 0U;
  RCC_PLL1FRACR = 0U;
  RCC_CR |= RCC_CR_HSEON;
  while ((RCC_CR & RCC_CR_HSERDY) == 0U) {
  }
  delay(100U);
  RCC_APB4ENR |= RCC_APB4ENR_SYSCFGEN;
  delay(10U);
  PWR_D3CR = (PWR_D3CR & ~(3UL << 14U)) | PWR_D3CR_VOS1;
  while ((PWR_D3CR & PWR_D3CR_VOSRDY) == 0U) {
  }
  FLASH_ACR = (4UL << 0U) | (2UL << 4U);
  RCC_D1CFGR = RCC_D1CFGR_HPRE_DIV2 | RCC_D1CFGR_D1PPRE_DIV2;
  RCC_PLLCKSELR = RCC_PLLCKSELR_PLLSRC_HSE | RCC_PLLCKSELR_DIVM1(2U);
  RCC_PLLCFGR = RCC_PLLCFGR_PLL1RGE(2U) | RCC_PLLCFGR_DIVP1EN;
  RCC_PLL1DIVR = ((240U - 1U) << 0U) | ((2U - 1U) << 9U);
  RCC_CR |= RCC_CR_PLL1ON;
  while ((RCC_CR & RCC_CR_PLL1RDY) == 0U) {
  }
  RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_PLL1;
  while ((RCC_CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL1) {
  }
}
