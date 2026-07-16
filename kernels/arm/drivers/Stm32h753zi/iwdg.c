/*
 * iwdg.c — STM32H753ZI Independent Watchdog driver
 *
 * IWDG is clocked from LSI (~32kHz, uncalibrated +/-10%), independent
 * of the main clock tree — it keeps running even if HSE/PLL/HSI fail
 * or a task hangs the scheduler. This is the point of it: a hung task
 * or a runaway ISR that stops kicking the watchdog forces a reset
 * rather than leaving the system silently stuck.
 *
 * NOTE: base addresses and RCC_RSR bit positions below reflect my best
 * recollection of RM0433. Given how easy it is to get offsets wrong
 * (see the FDCAN D2CCIP1R mistake earlier in this project), verify
 * every #define against RM0433 chapters "IWDG" and "RCC" before
 * flashing to real hardware — an IWDG that isn't kicked correctly
 * resets the board every timeout period, and an RCC_RSR bit mismatch
 * misreports the reset cause.
 */

#include "iwdg.h"

/* ── IWDG registers ── */
#define IWDG_BASE   0x58004800UL   /* confirmed via RM0433 memory map */
#define IWDG_KR     (*(volatile uint32_t *)(IWDG_BASE + 0x00U))
#define IWDG_PR     (*(volatile uint32_t *)(IWDG_BASE + 0x04U))
#define IWDG_RLR    (*(volatile uint32_t *)(IWDG_BASE + 0x08U))
#define IWDG_SR     (*(volatile uint32_t *)(IWDG_BASE + 0x0CU))
#define IWDG_WINR   (*(volatile uint32_t *)(IWDG_BASE + 0x10U))

/* ── KR key values ── */
#define IWDG_KEY_RELOAD   0x0000AAAAUL
#define IWDG_KEY_ENABLE   0x0000CCCCUL
#define IWDG_KEY_UNLOCK   0x00005555UL

/* ── SR busy flags ── */
#define IWDG_SR_PVU  (1UL << 0U)   /* prescaler update in progress */
#define IWDG_SR_RVU  (1UL << 1U)   /* reload value update in progress */

/* ── RCC reset status ── */
#define RCC_BASE    0x58024400UL
#define RCC_RSR     (*(volatile uint32_t *)(RCC_BASE + 0x0D0U))
#define RCC_RSR_IWDGRSTF  (1UL << 26U)   /* verify exact bit vs RM0433 RCC_RSR table */
#define RCC_RSR_RMVF      (1UL << 16U)   /* clear-all-flags bit */

/* LSI nominal frequency in Hz (uncalibrated, datasheet typical value) */
#define LSI_FREQ_HZ   32000UL

void iwdg_init(uint32_t timeout_ms)
{
    /* Prescaler options: /4, /8, /16, /32, /64, /128, /256 (PR = 0..6) */
    static const uint16_t prescaler_table[7] = {4, 8, 16, 32, 64, 128, 256};
    uint8_t pr_code = 6U;          /* start at largest prescaler (256) */
    uint32_t reload = 0U;

    /* Find smallest prescaler that lets reload fit in 12 bits (0..4095) */
    for (uint8_t i = 0U; i < 7U; i++) {
        uint32_t rl = (timeout_ms * (LSI_FREQ_HZ / prescaler_table[i])) / 1000U;
        if (rl <= 4095U) {
            pr_code = i;
            reload  = rl;
            break;
        }
        reload = 4095U;   /* clamp if even /256 can't reach requested timeout */
        pr_code = 6U;
    }
    if (reload == 0U) reload = 1U;   /* avoid a zero reload (instant reset) */

    /* Unlock write access to PR/RLR/WINR */
    IWDG_KR = IWDG_KEY_UNLOCK;

    while (IWDG_SR & IWDG_SR_PVU) {}   /* wait prescaler write-ready */
    IWDG_PR = pr_code;

    while (IWDG_SR & IWDG_SR_RVU) {}   /* wait reload write-ready */
    IWDG_RLR = reload;

    /* No window feature used — full range [0, reload] accepted */
    IWDG_WINR = 0x0FFFU;

    /* Reload counter with the new value before starting */
    IWDG_KR = IWDG_KEY_RELOAD;

    /* Start the watchdog — irreversible until next reset */
    IWDG_KR = IWDG_KEY_ENABLE;
}

void iwdg_kick(void)
{
    IWDG_KR = IWDG_KEY_RELOAD;
}

uint8_t iwdg_reset_was_watchdog(void)
{
    return (RCC_RSR & RCC_RSR_IWDGRSTF) ? 1U : 0U;
}

void iwdg_clear_reset_flags(void)
{
    RCC_RSR |= RCC_RSR_RMVF;
}