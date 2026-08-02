/*
 * tests/test_eth_link_monitor.c  standart 802.3 status register over MDIO to detect link up/down, speed,
 * and duplex mode. Blinks LD1 (PB0) will change 500ms on up if fast cacble is not connected !!!
 */

#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "eth.h"

#define PHY_ADDR  0U

#define PHY_REG_BASIC_STATUS   1U
#define PHY_REG_SPECIAL_CTRL_STATUS  31U 
#define PHY_BSR_LINK_STATUS_MASK  (1U << 2U)
#define PHY_SCSR_SPEED_DUPLEX_MASK  (0x1CU)
#define PHY_SCSR_SPEED_DUPLEX_SHIFT 2U

#define GPIOB_BASE   0x58020400UL
#define GPIOB_ODR    (*(volatile uint32_t *)(GPIOB_BASE + 0x14U))
#define GPIOB_MODER  (*(volatile uint32_t *)(GPIOB_BASE + 0x00U))
#define RCC_AHB4ENR  (*(volatile uint32_t *)(0x58024400UL + 0x0E0U))

#define LD1_PIN  0U

static const char *speed_duplex_str(uint8_t code)
{
    switch (code) {
        case 0x1U: return "10Mbps Half-Duplex";
        case 0x5U: return "10Mbps Full-Duplex";
        case 0x2U: return "100Mbps Half-Duplex";
        case 0x6U: return "100Mbps Full-Duplex";
        default:   return "Unknown/Not resolved yet";
    }
}

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    uart_init();

    uart_puts("TamgaOS STM32H753ZI — Ethernet PHY Link Monitor\r\n\r\n");

    eth_init_link_only();

    RCC_AHB4ENR |= (1UL << 1U);
    GPIOB_MODER &= ~(3UL << (LD1_PIN * 2U));
    GPIOB_MODER |=  (1UL << (LD1_PIN * 2U));

    uint16_t phy_id = mdio_read(PHY_ADDR, 2U);
    uart_printf("[ETH] PHY ID (reg 2) = 0x%X (expect a real value, "
                "not 0x0000 or 0xFFFF)\r\n\r\n", (unsigned int)phy_id);

    if (phy_id == 0xFFFFU || phy_id == 0x0000U) {
        uart_puts("[ETH] WARNING: PHY not responding at address 0 — "
                   "MDIO wiring, PHY address, or clock config may be "
                   "wrong. Continuing to poll anyway.\r\n\r\n");
    }

    uint8_t last_link_state = 0xFFU;
    
    for (;;) {
        uint16_t bsr = mdio_read(PHY_ADDR, PHY_REG_BASIC_STATUS);
        uint8_t link_up = (bsr & PHY_BSR_LINK_STATUS_MASK) ? 1U : 0U;

        if (link_up != last_link_state) {
            if (link_up) {
                uint16_t scsr = mdio_read(PHY_ADDR, PHY_REG_SPECIAL_CTRL_STATUS);
                uint8_t sd_code = (uint8_t)((scsr & PHY_SCSR_SPEED_DUPLEX_MASK)
                                             >> PHY_SCSR_SPEED_DUPLEX_SHIFT);

                uart_printf("[ETH] LINK UP — %s\r\n", speed_duplex_str(sd_code));
            } else {
                uart_puts("[ETH] LINK DOWN\r\n");
            }
            last_link_state = link_up;
        }
        GPIOB_ODR ^= (1UL << LD1_PIN);
        systick_delay_ms(link_up ? 500U : 100U);
    }

    return 0;
}