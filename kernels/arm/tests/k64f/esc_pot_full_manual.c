#include "mcg.h"
#include "systick.h"
#include "uart.h"
#include "pwm.h"
#include "adc.h"

#define MCG_BASE    0x40064000UL
#define MCG_C1      (*(volatile uint8_t *)(MCG_BASE + 0x00U))
#define MCG_C2      (*(volatile uint8_t *)(MCG_BASE + 0x01U))
#define MCG_C5      (*(volatile uint8_t *)(MCG_BASE + 0x04U))
#define MCG_C6      (*(volatile uint8_t *)(MCG_BASE + 0x05U))
#define MCG_S       (*(volatile uint8_t *)(MCG_BASE + 0x06U))

#define SIM_BASE      0x40047000UL
#define SIM_SCGC6     (*(volatile uint32_t *)(SIM_BASE + 0x103CU))
#define SIM_SCGC6_FTM0_MASK  (1UL << 24U)

#define FTM0_BASE   0x40038000UL
#define FTM0_SC     (*(volatile uint32_t *)(FTM0_BASE + 0x00U))
#define FTM0_CNT    (*(volatile uint32_t *)(FTM0_BASE + 0x04U))
#define FTM0_MOD    (*(volatile uint32_t *)(FTM0_BASE + 0x08U))
#define FTM_SC_CLKS_SYSTEM  (1UL << 3U)
#define FTM_SC_PS_DIV1      (0UL << 0U)

#define MOTOR_CH   0U
#define ADC_MAX    4095U
#define PULSE_MIN  1000U
#define PULSE_MAX  2000U
#define LOOP_MS    50U

static uint16_t pot_to_pulse_us(uint16_t raw)
{
    uint32_t span = PULSE_MAX - PULSE_MIN;
    return (uint16_t)(PULSE_MIN + ((uint32_t)raw * span) / ADC_MAX);
}

int main(void)
{
    mcg_init_120mhz();
    systick_init(120000000UL);
    uart_init(115200U);

    uart_puts("TamgaOS K64F — Full Manual Pot Control (Channel 0 / PTC1)\r\n");

    uart_printf("[CLK] MCG_C1=0x%x C2=0x%x C5=0x%x C6=0x%x S=0x%x\r\n",
                (unsigned int)MCG_C1, (unsigned int)MCG_C2,
                (unsigned int)MCG_C5, (unsigned int)MCG_C6,
                (unsigned int)MCG_S);

    SIM_SCGC6 |= SIM_SCGC6_FTM0_MASK;
    FTM0_SC  = 0U;
    FTM0_CNT = 0U;
    FTM0_MOD = 0xFFFFU;
    FTM0_SC  = FTM_SC_CLKS_SYSTEM | (7UL << 0U); 

    uint32_t c0 = FTM0_CNT;
    systick_delay_ms(100U);
    uint32_t c1 = FTM0_CNT;

    uart_printf("[CLK] FTM ticks in 100ms: %u (expected ~46875 if bus=60MHz)\r\n",
                (unsigned int)(c1 - c0));

    pwm_init();
    adc_init();

    if (adc_calibration_failed()) {
        uart_puts("[ADC] WARNING: calibration failed\r\n\r\n");
    }

    for (;;) {
        uint16_t raw   = adc_read_se12();
        uint16_t pulse = pot_to_pulse_us(raw);

        pwm_set_pulse_us_ch(MOTOR_CH, pulse);

        uart_printf("pot_raw=%u pulse=%uus\r\n",
                    (unsigned int)raw, (unsigned int)pulse);

        systick_delay_ms(LOOP_MS);
    }

    return 0;
}