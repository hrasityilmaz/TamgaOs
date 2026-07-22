/*
 * iwdg.c — STM32H753ZI Independent Watchdog driver
 *
 * IWDG is clocked from LSI (~32kHz, uncalibrated +/-10%), independent
 * of the main clock tree — it keeps running even if HSE/PLL/HSI fail
 * or a task hangs the scheduler. 
 *
 */

#include "iwdg.h"

#define IWDG_BASE   0x58004800UL 
#define IWDG_KR     (*(volatile uint32_t *)(IWDG_BASE + 0x00U))
#define IWDG_PR     (*(volatile uint32_t *)(IWDG_BASE + 0x04U))
#define IWDG_RLR    (*(volatile uint32_t *)(IWDG_BASE + 0x08U))
#define IWDG_SR     (*(volatile uint32_t *)(IWDG_BASE + 0x0CU))
#define IWDG_WINR   (*(volatile uint32_t *)(IWDG_BASE + 0x10U))
#define IWDG_KEY_RELOAD   0x0000AAAAUL
#define IWDG_KEY_ENABLE   0x0000CCCCUL
#define IWDG_KEY_UNLOCK   0x00005555UL
#define IWDG_SR_PVU  (1UL << 0U) 
#define IWDG_SR_RVU  (1UL << 1U)
#define RCC_BASE    0x58024400UL
#define RCC_RSR     (*(volatile uint32_t *)(RCC_BASE + 0x0D0U))
#define RCC_RSR_IWDGRSTF  (1UL << 26U) 
#define RCC_RSR_RMVF      (1UL << 16U)
#define LSI_FREQ_HZ   32000UL

void iwdg_init(uint32_t timeout_ms)
{
    static const uint16_t prescaler_table[7] = {4, 8, 16, 32, 64, 128, 256};
    uint8_t pr_code = 6U;  
    uint32_t reload = 0U;

    for (uint8_t i = 0U; i < 7U; i++) {
        uint32_t rl = (timeout_ms * (LSI_FREQ_HZ / prescaler_table[i])) / 1000U;
        if (rl <= 4095U) {
            pr_code = i;
            reload  = rl;
            break;
        }
        reload = 4095U;
        pr_code = 6U;
    }
    if (reload == 0U) reload = 1U; 

    IWDG_KR = IWDG_KEY_UNLOCK;
    while (IWDG_SR & IWDG_SR_PVU) {}
    IWDG_PR = pr_code;
    while (IWDG_SR & IWDG_SR_RVU) {} 
    IWDG_RLR = reload;
    IWDG_WINR = 0x0FFFU;
    IWDG_KR = IWDG_KEY_RELOAD;
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