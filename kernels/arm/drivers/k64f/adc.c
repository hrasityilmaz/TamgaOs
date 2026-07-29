/*
 * adc.c — K64F ADC0 single-channel polling driver
 *
 * Channel : PTB2 (Arduino A0), ADC0_SE12
 */

#include <stdint.h>

#define ADC0_BASE   0x4003B000UL
#define ADC0_SC1A   (*(volatile uint32_t *)(ADC0_BASE + 0x00U))
#define ADC0_CFG1   (*(volatile uint32_t *)(ADC0_BASE + 0x08U))
#define ADC0_CFG2   (*(volatile uint32_t *)(ADC0_BASE + 0x0CU))
#define ADC0_RA     (*(volatile uint32_t *)(ADC0_BASE + 0x10U))
#define ADC0_SC2    (*(volatile uint32_t *)(ADC0_BASE + 0x20U))
#define ADC0_SC3    (*(volatile uint32_t *)(ADC0_BASE + 0x24U))

#define ADC0_SC1_COCO_MASK  (1UL << 7U)
#define ADC0_SC3_CAL_MASK   (1UL << 7U)
#define ADC0_SC3_CALF_MASK  (1UL << 6U)

#define SIM_BASE     0x40047000UL
#define SIM_SCGC6    (*(volatile uint32_t *)(SIM_BASE + 0x103CU)) 
#define SIM_SCGC6_ADC0_MASK  (1UL << 27U) 

#define SIM_SCGC5    (*(volatile uint32_t *)(SIM_BASE + 0x1038U)) 
#define SIM_SCGC5_PORTB_MASK (1UL << 10U) 

#define PORTB_BASE   0x4004A000UL
#define PORTB_PCR2   (*(volatile uint32_t *)(PORTB_BASE + 0x08U))

#define ADC_CHANNEL_SE12  12U

void adc_init(void)
{
    SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK;
    SIM_SCGC6 |= SIM_SCGC6_ADC0_MASK;
    PORTB_PCR2 &= ~0x700UL;
    ADC0_CFG1 = (1UL << 2U);
    ADC0_CFG2 = 0U;
    ADC0_SC2 &= ~(1UL << 6U);
    ADC0_SC3 |= ADC0_SC3_CAL_MASK;
    while ((ADC0_SC3 & ADC0_SC3_CAL_MASK) != 0U) { } 

    if ((ADC0_SC3 & ADC0_SC3_CALF_MASK) != 0U) {
        ADC0_SC3 |= ADC0_SC3_CALF_MASK;
    }
}

uint8_t adc_calibration_failed(void)
{
    return (uint8_t)((ADC0_SC3 & ADC0_SC3_CALF_MASK) != 0U);
}

uint16_t adc_read_se12(void)
{
    ADC0_SC1A = ADC_CHANNEL_SE12;   /* AIEN=0, DIFF=0, ADCH=12 -> starts conversion */
    while ((ADC0_SC1A & ADC0_SC1_COCO_MASK) == 0U) { }
    return (uint16_t)(ADC0_RA & 0xFFFFU);
}