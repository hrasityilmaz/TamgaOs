/*
 * fdcan.c — STM32H753ZI FDCAN1 driver
 *
 * Mode   : Classic CAN, 1Mbit
 * Pins   :
 * PB8=RX (AF9)
 * PB9=TX (AF9)
 *
 */

#include "fdcan.h"
#include <stdint.h>
#include "uart.h"

#define RCC_BASE       0x58024400UL
#define RCC_AHB4ENR    (*(volatile uint32_t *)(RCC_BASE + 0x0E0U))  /* GPIOBEN bit1 */
#define RCC_APB1HENR   (*(volatile uint32_t *)(RCC_BASE + 0x0ECU))  /* FDCANEN bit8 */
#define RCC_D2CCIP1R   (*(volatile uint32_t *)(RCC_BASE + 0x050U))  /* FDCANSEL bits[29:28] */
#define GPIOB_BASE     0x58020400UL
#define GPIOB_MODER    (*(volatile uint32_t *)(GPIOB_BASE + 0x00U))
#define GPIOB_OSPEEDR  (*(volatile uint32_t *)(GPIOB_BASE + 0x08U))
#define GPIOB_PUPDR    (*(volatile uint32_t *)(GPIOB_BASE + 0x0CU))
#define GPIOB_AFRH     (*(volatile uint32_t *)(GPIOB_BASE + 0x24U))
#define FDCAN1_BASE    0x4000A000UL
#define FDCAN1_CCCR    (*(volatile uint32_t *)(FDCAN1_BASE + 0x018U))
#define FDCAN1_NBTP    (*(volatile uint32_t *)(FDCAN1_BASE + 0x01CU))
#define FDCAN1_TEST    (*(volatile uint32_t *)(FDCAN1_BASE + 0x010U))
#define FDCAN1_PSR     (*(volatile uint32_t *)(FDCAN1_BASE + 0x044U))
#define FDCAN1_RXF0C   (*(volatile uint32_t *)(FDCAN1_BASE + 0x0A0U))
#define FDCAN1_RXF0S   (*(volatile uint32_t *)(FDCAN1_BASE + 0x0A4U))
#define FDCAN1_RXF0A   (*(volatile uint32_t *)(FDCAN1_BASE + 0x0A8U))
#define FDCAN1_TXBC    (*(volatile uint32_t *)(FDCAN1_BASE + 0x0C0U))
#define FDCAN1_TXFQS   (*(volatile uint32_t *)(FDCAN1_BASE + 0x0C4U))
#define FDCAN1_TXBAR   (*(volatile uint32_t *)(FDCAN1_BASE + 0x0D0U))
#define FDCAN1_GFC     (*(volatile uint32_t *)(FDCAN1_BASE + 0x080U))

#define PSR_BO_MASK    (1UL << 7U)
#define MSG_RAM_BASE     0x4000AC00UL
#define RX_FIFO0_OFFSET  0x000U
#define TX_BUF_OFFSET    0x018U

typedef struct {
    volatile uint32_t hdr0;
    volatile uint32_t hdr1;
    volatile uint32_t data[2];
} fdcan_msg_t;

#define TX_BUF_ELEM ((fdcan_msg_t *)(MSG_RAM_BASE + TX_BUF_OFFSET))
#define CCCR_INIT  (1UL << 0U)
#define CCCR_CCE   (1UL << 1U)
#define CCCR_TEST  (1UL << 7U)
#define TEST_LBCK  (1UL << 4U)
#define FDCAN_TIMEOUT  100000U

static int fdcan_wait_init(uint32_t set)
{
    uint32_t t = FDCAN_TIMEOUT;
    while (t--) {
        if (set && (FDCAN1_CCCR & CCCR_INIT)) return 0;
        if (!set && !(FDCAN1_CCCR & CCCR_INIT)) return 0;
    }
    return -1;
}

void fdcan_init(void)
{
    RCC_AHB4ENR  |= (1UL << 1U);
    RCC_APB1HENR |= (1UL << 8U);
    RCC_D2CCIP1R &= ~(3UL << 28U);
    RCC_D2CCIP1R |=  (0UL << 28U);

    GPIOB_MODER &= ~((3UL << (8U * 2U)) | (3UL << (9U * 2U)));
    GPIOB_MODER |=   (2UL << (8U * 2U)) | (2UL << (9U * 2U));
    GPIOB_OSPEEDR |= (3UL << (8U * 2U)) | (3UL << (9U * 2U));
    GPIOB_PUPDR  &= ~((3UL << (8U * 2U)) | (3UL << (9U * 2U)));

    GPIOB_AFRH &= ~((0xFUL << 0U) | (0xFUL << 4U));
    GPIOB_AFRH |=   (9UL   << 0U) | (9UL   << 4U);

    FDCAN1_CCCR |= CCCR_INIT;
    if (fdcan_wait_init(1) < 0) {
        //uart_puts("[DEBUG] fdcan_wait_init(1) TIMED OUT\r\n");
        return;
    }
    FDCAN1_CCCR |= CCCR_CCE;
    FDCAN1_NBTP = ((0U  << 25U) |
                   (0U  << 16U) |
                   (4U  <<  8U) |
                   (1U  <<  0U)); 

    uart_printf("[DEBUG] NBTP=0x%x\r\n", (unsigned int)FDCAN1_NBTP);

    /* 5. LOOPBACK MODE — for self-test with logic analyzer.
     * Comment these two lines out for real bus operation. */
    //FDCAN1_CCCR |= CCCR_TEST;
    //FDCAN1_TEST  = TEST_LBCK;

    FDCAN1_GFC = 0x00000000U;
    FDCAN1_RXF0C = ((1U << 16U) |
                    ((RX_FIFO0_OFFSET / 4U) << 2U));

    FDCAN1_TXBC  = ((1U << 16U) |
                    ((TX_BUF_OFFSET / 4U) << 2U));

    FDCAN1_CCCR &= ~(CCCR_CCE | CCCR_INIT);
    int8_t wait_result = fdcan_wait_init(0);
    uint32_t psr = FDCAN1_PSR;
    uint32_t act = (psr >> 3U) & 0x3U;
}

void fdcan_debug_print_gpio(void)
{
    uart_printf("[DEBUG] GPIOB_MODER=0x%x GPIOB_AFRH=0x%x\r\n",
                (unsigned int)GPIOB_MODER, (unsigned int)GPIOB_AFRH);
}

uint8_t fdcan_is_bus_off(void)
{
    return (uint8_t)((FDCAN1_PSR & PSR_BO_MASK) != 0U);
}

uint32_t fdcan_get_psr(void)
{
    return FDCAN1_PSR;
}

int8_t fdcan_transmit(const fdcan_frame_t *frame)
{
    uint32_t t = FDCAN_TIMEOUT;
    while ((FDCAN1_TXFQS & (1UL << 21U)) && t--) {}
    if (!t) return -1;

    TX_BUF_ELEM->hdr0 = (frame->id & 0x7FFU) << 18U;
    TX_BUF_ELEM->hdr1 = (frame->dlc & 0xFU) << 16U;
    TX_BUF_ELEM->data[0] = ((uint32_t)frame->data[0]       |
                             ((uint32_t)frame->data[1] << 8U)  |
                             ((uint32_t)frame->data[2] << 16U) |
                             ((uint32_t)frame->data[3] << 24U));
    TX_BUF_ELEM->data[1] = ((uint32_t)frame->data[4]       |
                             ((uint32_t)frame->data[5] << 8U)  |
                             ((uint32_t)frame->data[6] << 16U) |
                             ((uint32_t)frame->data[7] << 24U));

    FDCAN1_TXBAR = (1UL << 0U);

    return 0;
}

uint8_t fdcan_rx_pending(void)
{
    return (uint8_t)((FDCAN1_RXF0S >> 0U) & 0x7FU);
}

int8_t fdcan_receive(fdcan_frame_t *frame)
{
    if (!fdcan_rx_pending()) return -1;

    uint32_t get_idx = (FDCAN1_RXF0S >> 8U) & 0x3FU;
    fdcan_msg_t *elem = (fdcan_msg_t *)(MSG_RAM_BASE +
                         RX_FIFO0_OFFSET +
                         get_idx * sizeof(fdcan_msg_t));

    frame->id  = (elem->hdr0 >> 18U) & 0x7FFU;
    frame->dlc = (elem->hdr1 >> 16U) & 0xFU;

    uint32_t d0 = elem->data[0];
    uint32_t d1 = elem->data[1];
    frame->data[0] = (uint8_t)(d0 >>  0U);
    frame->data[1] = (uint8_t)(d0 >>  8U);
    frame->data[2] = (uint8_t)(d0 >> 16U);
    frame->data[3] = (uint8_t)(d0 >> 24U);
    frame->data[4] = (uint8_t)(d1 >>  0U);
    frame->data[5] = (uint8_t)(d1 >>  8U);
    frame->data[6] = (uint8_t)(d1 >> 16U);
    frame->data[7] = (uint8_t)(d1 >> 24U);

    FDCAN1_RXF0A = get_idx;

    return 0;
}