/*
 * adc.c — STM32H753ZI ADC1 single-channel polling driver
 *
 * Channel : PA3, ADC1_INP15 (Table 8, PA3 row: "ADC12_INP15")
 * Mode    : Single conversion, software trigger, polling (no DMA,
 *           no interrupt) — simplest possible read-on-demand driver
 *
 * ADC1_BASE     = 0x40022000
 * RCC_AHB1ENR   = RCC_BASE + 0x0D8, ADC12EN = bit5
 * RCC_AHB4ENR   = RCC_BASE + 0x0E0, GPIOAEN = bit0
 * ADC_ISR       = ADC1_BASE + 0x00   (ADRDY=b0, EOC=b2, EOS=b3, OVR=b4)
 * ADC_CR        = ADC1_BASE + 0x08   (ADCAL=31, ADCALDIF=30, DEEPPWD=29, ADVREGEN=28, ADSTART=2, ADDIS=1, ADEN=0)
 * ADC_CFGR      = ADC1_BASE + 0x0C   (CONT=13)
 * ADC_SMPR2     = ADC1_BASE + 0x18   (SMP15 = bit[17:15])
 * ADC_PCSEL     = ADC1_BASE + 0x1C   (bit15 for channel 15)
 * ADC_SQR1      = ADC1_BASE + 0x30   (SQ1[4:0]=bit[10:6], L[3:0]=bit[3:0])
 * ADC_DR        = ADC1_BASE + 0x40
 */

#include <stdint.h>

#define RCC_D3CCIPR   (*(volatile uint32_t *)(RCC_BASE + 0x058U))
#define RCC_D3CCIPR_ADCSEL_PERCK  (2UL << 16U)
#define ADC1_BASE     0x40022000UL
#define ADC_ISR       (*(volatile uint32_t *)(ADC1_BASE + 0x00U))
#define ADC_CR        (*(volatile uint32_t *)(ADC1_BASE + 0x08U))
#define ADC_CFGR      (*(volatile uint32_t *)(ADC1_BASE + 0x0CU))
#define ADC_SMPR2     (*(volatile uint32_t *)(ADC1_BASE + 0x18U))
#define ADC_PCSEL     (*(volatile uint32_t *)(ADC1_BASE + 0x1CU))
#define ADC_SQR1      (*(volatile uint32_t *)(ADC1_BASE + 0x30U))
#define ADC_DR        (*(volatile uint32_t *)(ADC1_BASE + 0x40U))

#define ADC_ISR_ADRDY_MASK  (1UL << 0U)
#define ADC_ISR_EOC_MASK    (1UL << 2U)

#define ADC_CR_ADEN_MASK      (1UL << 0U)
#define ADC_CR_ADDIS_MASK     (1UL << 1U)
#define ADC_CR_ADSTART_MASK   (1UL << 2U)
#define ADC_CR_ADCALDIF_MASK  (1UL << 30U)
#define ADC_CR_ADCAL_MASK     (1UL << 31U)
#define ADC_CR_ADVREGEN_MASK  (1UL << 28U)
#define ADC_CR_DEEPPWD_MASK   (1UL << 29U)
#define ADC_CR_ADCALLIN_MASK  (1UL << 16U)

#define RCC_BASE      0x58024400UL
#define RCC_AHB1ENR   (*(volatile uint32_t *)(RCC_BASE + 0x0D8U))
#define RCC_AHB4ENR   (*(volatile uint32_t *)(RCC_BASE + 0x0E0U))
#define RCC_AHB1ENR_ADC12EN_MASK (1UL << 5U)
#define RCC_AHB4ENR_GPIOAEN_MASK (1UL << 0U)

#define GPIOA_BASE    0x58020000UL
#define GPIOA_MODER   (*(volatile uint32_t *)(GPIOA_BASE + 0x00U))

#define ADC_CHANNEL_15   15U

static void adc_delay_short(void)
{
    for (volatile uint32_t i = 0; i < 4000U; i++) { }
}

void adc_init(void)
{   
    RCC_D3CCIPR &= ~(3UL << 16U);
    RCC_D3CCIPR |= RCC_D3CCIPR_ADCSEL_PERCK;
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOAEN_MASK;
    RCC_AHB1ENR |= RCC_AHB1ENR_ADC12EN_MASK;
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOAEN_MASK;
    RCC_AHB1ENR |= RCC_AHB1ENR_ADC12EN_MASK;
    GPIOA_MODER |= (3UL << (3U * 2U));
    ADC_CR &= ~ADC_CR_DEEPPWD_MASK;
    ADC_CR |= ADC_CR_ADVREGEN_MASK;
    adc_delay_short();
    ADC_CR &= ~ADC_CR_ADCALDIF_MASK;
    ADC_CR &= ~ADC_CR_ADCALLIN_MASK;
    ADC_CR |= ADC_CR_ADCAL_MASK;
    while ((ADC_CR & ADC_CR_ADCAL_MASK) != 0U) { }
    ADC_PCSEL |= (1UL << ADC_CHANNEL_15);
    ADC_SMPR2 &= ~(0x7UL << 15U);
    ADC_SMPR2 |=  (0x4UL << 15U);
    ADC_CFGR &= ~(1UL << 13U);
    ADC_SQR1 = (ADC_CHANNEL_15 << 6U) | (0UL << 0U);

    ADC_CR |= ADC_CR_ADEN_MASK;
    while ((ADC_ISR & ADC_ISR_ADRDY_MASK) == 0U) { }
}

uint16_t adc_read_channel15(void)
{
    ADC_CR |= ADC_CR_ADSTART_MASK;
    while ((ADC_ISR & ADC_ISR_EOC_MASK) == 0U) { }
    return (uint16_t)ADC_DR;
}