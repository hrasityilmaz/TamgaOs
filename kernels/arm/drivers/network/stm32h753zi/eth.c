/*
 * eth.c - STM32H753ZI Ethernet MAC+DMA driver, RMII interface,
 * register-level, no HAL. Raw Ethernet frames only (no IP/UDP/TCP).
 */

#include "eth.h"
#include <string.h>

#define ETH_BASE        0x40028000UL
#define ETH_MACCR       (*(volatile uint32_t *)(ETH_BASE + 0x0000U))
#define ETH_MACA0HR     (*(volatile uint32_t *)(ETH_BASE + 0x0300U))
#define ETH_MACA0LR     (*(volatile uint32_t *)(ETH_BASE + 0x0304U))
#define ETH_MACMDIOAR   (*(volatile uint32_t *)(ETH_BASE + 0x0200U))
#define ETH_MACMDIODR   (*(volatile uint32_t *)(ETH_BASE + 0x0204U))
#define ETH_MTLRXQOMR   (*(volatile uint32_t *)(ETH_BASE + 0x0D30U))

#define ETH_DMAMR       (*(volatile uint32_t *)(ETH_BASE + 0x1000U))
#define ETH_DMACCR      (*(volatile uint32_t *)(ETH_BASE + 0x1100U))
#define ETH_DMACTXCR    (*(volatile uint32_t *)(ETH_BASE + 0x1104U))
#define ETH_DMACRXCR    (*(volatile uint32_t *)(ETH_BASE + 0x1108U))
#define ETH_DMACTXDLAR  (*(volatile uint32_t *)(ETH_BASE + 0x1114U))
#define ETH_DMACRXDLAR  (*(volatile uint32_t *)(ETH_BASE + 0x111CU))
#define ETH_DMACTXDTPR  (*(volatile uint32_t *)(ETH_BASE + 0x1120U))
#define ETH_DMACRXDTPR  (*(volatile uint32_t *)(ETH_BASE + 0x1128U))
#define ETH_DMACTXRLR   (*(volatile uint32_t *)(ETH_BASE + 0x112CU))
#define ETH_DMACRXRLR   (*(volatile uint32_t *)(ETH_BASE + 0x1130U))
#define ETH_DMACSR      (*(volatile uint32_t *)(ETH_BASE + 0x1160U))

#define ETH_MACCR_LM_MASK   (1UL << 12U)
#define ETH_MACCR_DM_MASK   (1UL << 13U)
#define ETH_MACCR_FES_MASK  (1UL << 14U)
#define ETH_MACCR_TE_MASK   (1UL << 1U)
#define ETH_MACCR_RE_MASK   (1UL << 0U)

#define ETH_MACA0HR_AE_MASK   (1UL << 31U)

#define ETH_MTLRXQOMR_RSF_MASK  (1UL << 5U)

#define ETH_DMAMR_SWR_MASK  (1UL << 0U)
#define ETH_DMACTXCR_ST_MASK (1UL << 0U)
#define ETH_DMACRXCR_SR_MASK (1UL << 0U)
#define ETH_DMACRXCR_RBSZ_SHIFT  1U
#define ETH_DMACRXCR_RXPBL_SHIFT 16U

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
#define ETH_DMA_RAM __attribute__((section(".axi_bss")))

#define MPU_CTRL (*(volatile uint32_t *)0xE000ED94U)
#define MPU_RNR  (*(volatile uint32_t *)0xE000ED98U)
#define MPU_RBAR (*(volatile uint32_t *)0xE000ED9CU)
#define MPU_RASR (*(volatile uint32_t *)0xE000EDA0U)

#define ETH_MPU_REGION       6U 

#define ETH_MPU_RASR_XN           (1UL << 28U)
#define ETH_MPU_RASR_AP_RW        (3UL << 24U)
#define ETH_MPU_RASR_TEX_NORMAL_NC (1UL << 19U)
#define ETH_MPU_RASR_SIZE_8K      (12UL << 1U)
#define ETH_MPU_RASR_ENABLE       (1UL << 0U)

static void gpio_set_af_eth(uint32_t port_base, uint8_t pin)
{
    volatile uint32_t *moder = (volatile uint32_t *)(port_base + GPIO_MODER_OFFSET);
    *moder &= ~(3UL << (pin * 2U));
    *moder |=  (2UL << (pin * 2U));   /* AF mode = 10 */

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

#define MDIO_CR_VALUE  4UL   /* confirmed: AHB1=240MHz -> CSR/102 ~2.35MHz */

uint16_t mdio_read(uint8_t phy_addr, uint8_t reg_addr)
{
    ETH_MACMDIOAR = ((uint32_t)phy_addr << 21U)
                   | ((uint32_t)reg_addr << 16U)
                   | (MDIO_CR_VALUE << 8U)
                   | (3UL << 2U)   /* GOC=11, Read */
                   | 1UL;          /* MB */

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

#define ETH_NUM_TX_DESC  2U
#define ETH_NUM_RX_DESC  4U
#define ETH_BUF_SIZE     1536U

typedef struct {
    volatile uint32_t des0;
    volatile uint32_t des1;
    volatile uint32_t des2;
    volatile uint32_t des3;
} eth_descriptor_t;

static eth_descriptor_t s_tx_desc[ETH_NUM_TX_DESC] ETH_DMA_RAM __attribute__((aligned(32)));
static eth_descriptor_t s_rx_desc[ETH_NUM_RX_DESC] ETH_DMA_RAM __attribute__((aligned(32)));
static uint8_t s_tx_buf[ETH_NUM_TX_DESC][ETH_BUF_SIZE] ETH_DMA_RAM __attribute__((aligned(32)));
static uint8_t s_rx_buf[ETH_NUM_RX_DESC][ETH_BUF_SIZE] ETH_DMA_RAM __attribute__((aligned(32)));
static uint8_t s_tx_index = 0U;
static uint8_t s_rx_index = 0U;

static void eth_configure_dma_region_noncacheable(void)
{
    uint32_t base = (uint32_t)(uintptr_t)&s_tx_desc[0];
    /* base must be aligned to the region size (8KB) — enforced by
     * linker.ld's `. = ALIGN(8192);` on the .axi_bss section */

    MPU_CTRL &= ~(1UL << 0U);
    __asm volatile("dsb");

    MPU_RNR = ETH_MPU_REGION;
    MPU_RBAR = base;
    MPU_RASR = ETH_MPU_RASR_XN
             | ETH_MPU_RASR_AP_RW
             | ETH_MPU_RASR_TEX_NORMAL_NC
             | ETH_MPU_RASR_SIZE_8K
             | ETH_MPU_RASR_ENABLE;

    __asm volatile("dsb");

    MPU_CTRL |= (1UL << 0U) | (1UL << 2U);   /* ENABLE | PRIVDEFENA */

    __asm volatile("dsb");
    __asm volatile("isb");
}

void eth_init(uint8_t loopback_mode)
{
    eth_configure_dma_region_noncacheable();

    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOAEN_MASK | RCC_AHB4ENR_GPIOBEN_MASK
                 | RCC_AHB4ENR_GPIOCEN_MASK | RCC_AHB4ENR_GPIOGEN_MASK;
    RCC_APB4ENR |= RCC_APB4ENR_SYSCFGEN_MASK;

    /* RMII selection MUST happen before enabling the ETH clocks */
    SYSCFG_PMCR = (SYSCFG_PMCR & ~SYSCFG_PMCR_EPIS_MASK) | SYSCFG_PMCR_EPIS_RMII;

    RCC_AHB1ENR |= RCC_AHB1ENR_ETH1MACEN_MASK
                 | RCC_AHB1ENR_ETH1TXEN_MASK
                 | RCC_AHB1ENR_ETH1RXEN_MASK;

    gpio_set_af_eth(GPIOA_BASE, 1U);   /* PA1  REF_CLK */
    gpio_set_af_eth(GPIOA_BASE, 2U);   /* PA2  MDIO */
    gpio_set_af_eth(GPIOA_BASE, 7U);   /* PA7  CRS_DV */
    gpio_set_af_eth(GPIOB_BASE, 13U);  /* PB13 TXD1 */
    gpio_set_af_eth(GPIOC_BASE, 1U);   /* PC1  MDC */
    gpio_set_af_eth(GPIOC_BASE, 4U);   /* PC4  RXD0 */
    gpio_set_af_eth(GPIOC_BASE, 5U);   /* PC5  RXD1 */
    gpio_set_af_eth(GPIOG_BASE, 11U);  /* PG11 TX_EN */
    gpio_set_af_eth(GPIOG_BASE, 13U);  /* PG13 TXD0 */

    ETH_DMAMR |= ETH_DMAMR_SWR_MASK;
    uint32_t timeout = 100000U;
    while ((ETH_DMAMR & ETH_DMAMR_SWR_MASK) != 0U) {
        if (--timeout == 0U) return;
    }

    ETH_DMACCR = 0U;   /* DSL=0, contiguous descriptors, explicit */

    uint32_t maccr = ETH_MACCR_FES_MASK | ETH_MACCR_DM_MASK;
    if (loopback_mode) {
        maccr |= ETH_MACCR_LM_MASK;   /* NOTE: loopback mode never
                                          fully verified — see "KNOWN
                                          LIMITATION" at top of file */
    }
    ETH_MACCR = maccr;
    ETH_MACA0HR = ETH_MACA0HR_AE_MASK | 0x0100U;
    ETH_MACA0LR = 0x00000002U;

    /* --- TX ring setup --- */
    memset(s_tx_desc, 0, sizeof(s_tx_desc));
    for (uint8_t i = 0U; i < ETH_NUM_TX_DESC; i++) {
        s_tx_desc[i].des0 = (uint32_t)(uintptr_t)&s_tx_buf[i][0];
    }
    ETH_DMACTXDLAR = (uint32_t)(uintptr_t)&s_tx_desc[0];
    ETH_DMACTXRLR  = ETH_NUM_TX_DESC - 1U;
    ETH_DMACTXDTPR = (uint32_t)(uintptr_t)&s_tx_desc[0];
    ETH_MTLRXQOMR |= ETH_MTLRXQOMR_RSF_MASK;   /* Store-and-Forward — bug fix #6 */

    memset(s_rx_desc, 0, sizeof(s_rx_desc));
    for (uint8_t i = 0U; i < ETH_NUM_RX_DESC; i++) {
        s_rx_desc[i].des0 = (uint32_t)(uintptr_t)&s_rx_buf[i][0];
        s_rx_desc[i].des3 = (1UL << 31U)   /* OWN=1, DMA owns it */
                          | (1UL << 30U)   /* IOC=1 */
                          | (1UL << 24U);  /* BUF1V=1 */
    }
    ETH_DMACRXDLAR = (uint32_t)(uintptr_t)&s_rx_desc[0];
    ETH_DMACRXRLR  = ETH_NUM_RX_DESC - 1U;
    ETH_DMACRXDTPR = (uint32_t)(uintptr_t)&s_rx_desc[ETH_NUM_RX_DESC - 1U];

    ETH_DMACTXCR |= ETH_DMACTXCR_ST_MASK;
    ETH_DMACRXCR = (1UL << ETH_DMACRXCR_RXPBL_SHIFT)                    /* RXPBL=1 */
                 | ((uint32_t)ETH_BUF_SIZE << ETH_DMACRXCR_RBSZ_SHIFT)   /* RBSZ */
                 | ETH_DMACRXCR_SR_MASK;                                  /* SR=1 */

    ETH_MACCR |= ETH_MACCR_TE_MASK | ETH_MACCR_RE_MASK;
}

int8_t eth_transmit(const uint8_t *data, uint16_t len)
{
    if (len > ETH_BUF_SIZE) {
        return -1;
    }
    eth_descriptor_t *d = &s_tx_desc[s_tx_index];
    uint32_t timeout = 100000U;
    while ((d->des3 & (1UL << 31U)) != 0U) {
        if (--timeout == 0U) return -1;
    }

    memcpy(&s_tx_buf[s_tx_index][0], data, len);

    d->des0 = (uint32_t)(uintptr_t)&s_tx_buf[s_tx_index][0];
    d->des1 = 0U;
    d->des2 = (1UL << 31U) | ((uint32_t)len & 0x3FFFU);
    d->des3 = (1UL << 31U)
            | (1UL << 29U)
            | (1UL << 28U)
            | ((uint32_t)len & 0x7FFFU);

    ETH_DMACTXDTPR = (uint32_t)(uintptr_t)d;

    s_tx_index = (uint8_t)((s_tx_index + 1U) % ETH_NUM_TX_DESC);

    return 0;
}

int8_t eth_receive(uint8_t *buf, uint16_t max_len, uint16_t *out_len)
{
    eth_descriptor_t *d = &s_rx_desc[s_rx_index];

    if ((d->des3 & (1UL << 31U)) != 0U) {
        return -1;   /* still owned by DMA, nothing new */
    }

    uint16_t pkt_len = (uint16_t)(d->des3 & 0x7FFFU);
    if (pkt_len > max_len) {
        pkt_len = max_len;
    }
    memcpy(buf, &s_rx_buf[s_rx_index][0], pkt_len);
    *out_len = pkt_len;

    /* Hand this descriptor back to the DMA */
    d->des3 = (1UL << 31U) | (1UL << 30U) | (1UL << 24U);

    ETH_DMACRXDTPR = (uint32_t)(uintptr_t)d;

    s_rx_index = (uint8_t)((s_rx_index + 1U) % ETH_NUM_RX_DESC);

    return 0;
}

uint32_t eth_debug_read_dmacsr(void)
{
    return ETH_DMACSR;
}

void eth_debug_clear_dmacsr(uint32_t mask)
{
    ETH_DMACSR = mask;
}

uint32_t eth_debug_read_rx_des3(uint8_t index)
{
    if (index >= ETH_NUM_RX_DESC) return 0xFFFFFFFFU;
    return s_rx_desc[index].des3;
}

void eth_debug_dump_registers(void)
{
    extern void uart_printf(const char *fmt, ...);

    uart_printf("[ETH-DUMP] MACCR=0x%X\r\n", (unsigned int)ETH_MACCR);
    uart_printf("[ETH-DUMP] DMAMR=0x%X\r\n", (unsigned int)ETH_DMAMR);
    uart_printf("[ETH-DUMP] DMACCR=0x%X\r\n", (unsigned int)ETH_DMACCR);
    uart_printf("[ETH-DUMP] DMACTXCR=0x%X\r\n", (unsigned int)ETH_DMACTXCR);
    uart_printf("[ETH-DUMP] DMACRXCR=0x%X\r\n", (unsigned int)ETH_DMACRXCR);
    uart_printf("[ETH-DUMP] DMACRXDLAR=0x%X\r\n", (unsigned int)ETH_DMACRXDLAR);
    uart_printf("[ETH-DUMP] DMACRXRLR=0x%X\r\n", (unsigned int)ETH_DMACRXRLR);
    uart_printf("[ETH-DUMP] DMACRXDTPR=0x%X\r\n", (unsigned int)ETH_DMACRXDTPR);
    uart_printf("[ETH-DUMP] MTLRXQOMR=0x%X\r\n", (unsigned int)ETH_MTLRXQOMR);
    uart_printf("[ETH-DUMP] DMACSR=0x%X\r\n", (unsigned int)ETH_DMACSR);
}