/*
 * eth.c  talk to the PHY over MDIO and read link status/speed/duplex
 * will come more ı hope soon ...
 */

#include "eth.h"

#define ETH_BASE        0x40028000UL
#define ETH_MACCR       (*(volatile uint32_t *)(ETH_BASE + 0x0000U))
#define ETH_MACMDIOAR   (*(volatile uint32_t *)(ETH_BASE + 0x0200U))
#define ETH_MACMDIODR   (*(volatile uint32_t *)(ETH_BASE + 0x0204U))
#define ETH_MACCR_DM_MASK   (1UL << 13U)
#define ETH_MACCR_FES_MASK  (1UL << 14U)
#define SYSCFG_BASE     0x58000400UL
#define SYSCFG_PMCR     (*(volatile uint32_t *)(SYSCFG_BASE + 0x04U))
#define SYSCFG_PMCR_EPIS_RMII  (4UL << 21U)
#define SYSCFG_PMCR_EPIS_MASK  (7UL << 21U)
#define RCC_BASE        0x58024400UL
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x0D8U))
#define RCC_AHB4ENR     (*(volatile uint32_t *)(RCC_BASE + 0x0E0U))
#define RCC_APB4ENR     (*(volatile uint32_t *)(RCC_BASE + 0x0F4U))
#define RCC_AHB1ENR_ETH1MACEN_MASK (1UL << 15U)
#define RCC_AHB1ENR_ETH1TXEN_MASK  (1UL << 16U)
#define RCC_AHB1ENR_ETH1RXEN_MASK  (1UL << 17U)
#define RCC_AHB4ENR_GPIOAEN_MASK (1UL << 0U)
#define RCC_AHB4ENR_GPIOBEN_MASK (1UL << 1U)
#define RCC_AHB4ENR_GPIOCEN_MASK (1UL << 2U)
#define RCC_AHB4ENR_GPIOGEN_MASK (1UL << 6U)
#define RCC_APB4ENR_SYSCFGEN_MASK (1UL << 1U)
#define GPIOA_BASE  0x58020000UL
#define GPIOB_BASE  0x58020400UL
#define GPIOC_BASE  0x58020800UL
#define GPIOG_BASE  0x58021800UL
#define GPIO_MODER_OFFSET  0x00U
#define GPIO_AFRL_OFFSET   0x20U
#define GPIO_AFRH_OFFSET   0x24U
#define ETH_AF  11UL

static void gpio_set_af_eth(uint32_t port_base, uint8_t pin)
{
    volatile uint32_t *moder = (volatile uint32_t *)(port_base + GPIO_MODER_OFFSET);
    *moder &= ~(3UL << (pin * 2U));
    *moder |=  (2UL << (pin * 2U));

    if (pin < 8U) {
        volatile uint32_t *afrl = (volatile uint32_t *)(port_base + GPIO_AFRL_OFFSET);
        *afrl &= ~(0xFUL << (pin * 4U));
        *afrl |=  (ETH_AF << (pin * 4U));
    } else {
        volatile uint32_t *afrh = (volatile uint32_t *)(port_base + GPIO_AFRH_OFFSET);
        uint8_t shift = (uint8_t)((pin - 8U) * 4U);
        *afrh &= ~(0xFUL << shift);
        *afrh |=  (ETH_AF << shift);
    }
}

#define MDIO_CR_VALUE  4UL 

uint16_t mdio_read(uint8_t phy_addr, uint8_t reg_addr)
{
    ETH_MACMDIOAR = ((uint32_t)phy_addr << 21U)
                   | ((uint32_t)reg_addr << 16U)
                   | (MDIO_CR_VALUE << 8U)
                   | (3UL << 2U)
                   | 1UL;         

    uint32_t timeout = 100000U;
    while ((ETH_MACMDIOAR & 1UL) != 0U) {
        if (--timeout == 0U) return 0xFFFFU;
    }

    return (uint16_t)(ETH_MACMDIODR & 0xFFFFU);
}

void mdio_write(uint8_t phy_addr, uint8_t reg_addr, uint16_t value)
{
    ETH_MACMDIODR = value;

    ETH_MACMDIOAR = ((uint32_t)phy_addr << 21U)
                   | ((uint32_t)reg_addr << 16U)
                   | (MDIO_CR_VALUE << 8U)
                   | (1UL << 2U)   /* GOC=01, Write */
                   | 1UL;          /* MB */

    uint32_t timeout = 100000U;
    while ((ETH_MACMDIOAR & 1UL) != 0U) {
        if (--timeout == 0U) return;
    }
}

void eth_init_link_only(void)
{
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOAEN_MASK | RCC_AHB4ENR_GPIOBEN_MASK
                 | RCC_AHB4ENR_GPIOCEN_MASK | RCC_AHB4ENR_GPIOGEN_MASK;
    RCC_APB4ENR |= RCC_APB4ENR_SYSCFGEN_MASK;

    SYSCFG_PMCR = (SYSCFG_PMCR & ~SYSCFG_PMCR_EPIS_MASK) | SYSCFG_PMCR_EPIS_RMII;

    RCC_AHB1ENR |= RCC_AHB1ENR_ETH1MACEN_MASK
                 | RCC_AHB1ENR_ETH1TXEN_MASK
                 | RCC_AHB1ENR_ETH1RXEN_MASK;

    gpio_set_af_eth(GPIOA_BASE, 1U);
    gpio_set_af_eth(GPIOA_BASE, 2U);
    gpio_set_af_eth(GPIOA_BASE, 7U);
    gpio_set_af_eth(GPIOB_BASE, 13U);
    gpio_set_af_eth(GPIOC_BASE, 1U);
    gpio_set_af_eth(GPIOC_BASE, 4U);
    gpio_set_af_eth(GPIOC_BASE, 5U);
    gpio_set_af_eth(GPIOG_BASE, 11U);
    gpio_set_af_eth(GPIOG_BASE, 13U);

    ETH_MACCR = ETH_MACCR_FES_MASK | ETH_MACCR_DM_MASK;
}