/*
 * pwm.c — K64F FTM0/FTM3 PWM driver
 *
 * Motor 1: PTC1 → FTM0_CH0 (ALT4)
 * Motor 2: PTC5 → FTM0_CH2 (ALT7)
 * Motor 3: PTC8 → FTM3_CH4 (ALT3)
 * Motor 4: PTC9 → FTM3_CH5 (ALT3)
 *
 */

#include <stdint.h>
#include "uart.h"

#define FTM0_BASE   0x40038000UL
#define FTM3_BASE   0x400B9000UL
#define SIM_BASE      0x40047000UL
#define PORTC_BASE    0x4004B000UL
#define FTM_BUS_CLOCK_HZ   60000000UL

#define FTM0_SC     (*(volatile uint32_t *)(FTM0_BASE + 0x00U))
#define FTM0_CNT    (*(volatile uint32_t *)(FTM0_BASE + 0x04U))
#define FTM0_MOD    (*(volatile uint32_t *)(FTM0_BASE + 0x08U))
#define FTM0_C0SC   (*(volatile uint32_t *)(FTM0_BASE + 0x0CU))
#define FTM0_C0V    (*(volatile uint32_t *)(FTM0_BASE + 0x10U))
#define FTM0_C2SC   (*(volatile uint32_t *)(FTM0_BASE + 0x1CU))
#define FTM0_C2V    (*(volatile uint32_t *)(FTM0_BASE + 0x20U))
#define FTM3_SC     (*(volatile uint32_t *)(FTM3_BASE + 0x00U))
#define FTM3_CNT    (*(volatile uint32_t *)(FTM3_BASE + 0x04U))
#define FTM3_MOD    (*(volatile uint32_t *)(FTM3_BASE + 0x08U))
#define FTM3_C4SC   (*(volatile uint32_t *)(FTM3_BASE + 0x2CU))
#define FTM3_C4V    (*(volatile uint32_t *)(FTM3_BASE + 0x30U))
#define FTM3_C5SC   (*(volatile uint32_t *)(FTM3_BASE + 0x34U))
#define FTM3_C5V    (*(volatile uint32_t *)(FTM3_BASE + 0x38U))
#define SIM_SCGC3     (*(volatile uint32_t *)(SIM_BASE + 0x1030U))
#define SIM_SCGC5     (*(volatile uint32_t *)(SIM_BASE + 0x1038U))
#define SIM_SCGC6     (*(volatile uint32_t *)(SIM_BASE + 0x103CU))
#define SIM_SCGC3_FTM3_MASK  (1UL << 25U)
#define SIM_SCGC5_PORTC_MASK (1UL << 11U)
#define SIM_SCGC6_FTM0_MASK  (1UL << 24U)
#define PORTC_PCR1    (*(volatile uint32_t *)(PORTC_BASE + 0x04U))
#define PORTC_PCR5    (*(volatile uint32_t *)(PORTC_BASE + 0x14U))
#define PORTC_PCR8    (*(volatile uint32_t *)(PORTC_BASE + 0x20U))
#define PORTC_PCR9    (*(volatile uint32_t *)(PORTC_BASE + 0x24U))
#define FTM_SC_CLKS_SYSTEM  (1UL << 3U)
#define FTM_SC_PS_DIV32     (5UL << 0U)
#define FTM_CnSC_PWM_HIGHTRUE  ((1UL << 5U) | (1UL << 3U))

#define FTM_EFF_CLOCK_HZ   (FTM_BUS_CLOCK_HZ / 32UL)
#define PWM_PERIOD_TICKS(freq_hz)   ((FTM_EFF_CLOCK_HZ / (freq_hz)) - 1UL)
#define PWM_US_TO_TICKS(us)         (((uint32_t)(us) * 15UL) / 8UL)

void pwm_init(void)
{
    SIM_SCGC6 |= SIM_SCGC6_FTM0_MASK;
    SIM_SCGC3 |= SIM_SCGC3_FTM3_MASK;
    SIM_SCGC5 |= SIM_SCGC5_PORTC_MASK;
    PORTC_PCR1 = (PORTC_PCR1 & ~0x700UL) | (4UL << 8U);
    PORTC_PCR5 = (PORTC_PCR5 & ~0x700UL) | (7UL << 8U);
    PORTC_PCR8 = (PORTC_PCR8 & ~0x700UL) | (3UL << 8U);
    PORTC_PCR9 = (PORTC_PCR9 & ~0x700UL) | (3UL << 8U);

    uint32_t period_ticks = PWM_PERIOD_TICKS(50UL);   /* 37499 */
    uint32_t start_ticks  = PWM_US_TO_TICKS(1000U);   /* 1875 */

    FTM0_SC = 0U;
    FTM0_MOD = period_ticks;
    FTM0_C0SC = FTM_CnSC_PWM_HIGHTRUE;
    FTM0_C0V  = start_ticks;
    FTM0_C2SC = FTM_CnSC_PWM_HIGHTRUE;
    FTM0_C2V  = start_ticks;
    FTM0_SC = FTM_SC_CLKS_SYSTEM | FTM_SC_PS_DIV32;

    FTM3_SC = 0U;
    FTM3_MOD = period_ticks;
    FTM3_C4SC = FTM_CnSC_PWM_HIGHTRUE;
    FTM3_C4V  = start_ticks;
    FTM3_C5SC = FTM_CnSC_PWM_HIGHTRUE;
    FTM3_C5V  = start_ticks;
    FTM3_SC = FTM_SC_CLKS_SYSTEM | FTM_SC_PS_DIV32;

    //uart_printf("[PWM-K64F] init done: MOD=%u start_ticks=%u\r\n", (unsigned int)period_ticks, (unsigned int)start_ticks);
    //uart_printf("[PWM-K64F] FTM0_SC=0x%x FTM0_C0SC=0x%x FTM0_C0V=%u\r\n", (unsigned int)FTM0_SC, (unsigned int)FTM0_C0SC, (unsigned int)FTM0_C0V);
    //uart_printf("[PWM-K64F] PORTC_PCR1=0x%x\r\n", (unsigned int)PORTC_PCR1);
}

void pwm_set_pulse_us_ch(uint8_t channel, uint16_t pulse_us)
{
    if (pulse_us < 1000U) pulse_us = 1000U;
    if (pulse_us > 2000U) pulse_us = 2000U;

    uint32_t ticks = PWM_US_TO_TICKS(pulse_us);

    switch (channel) {
        case 0U: FTM0_C0V = ticks; break;
        case 1U: FTM0_C2V = ticks; break;
        case 2U: FTM3_C4V = ticks; break;
        case 3U: FTM3_C5V = ticks; break;
        default: break;
    }
}

void pwm_debug_print_regs(void)
{
    uart_printf("[PWM-DEBUG] FTM0_SC=0x%x FTM0_MOD=%u FTM0_C0V=%u FTM0_C2V=%u\r\n",
                (unsigned int)FTM0_SC, (unsigned int)FTM0_MOD,
                (unsigned int)FTM0_C0V, (unsigned int)FTM0_C2V);
    uart_printf("[PWM-DEBUG] FTM3_SC=0x%x FTM3_MOD=%u FTM3_C4V=%u FTM3_C5V=%u\r\n",
                (unsigned int)FTM3_SC, (unsigned int)FTM3_MOD,
                (unsigned int)FTM3_C4V, (unsigned int)FTM3_C5V);
}