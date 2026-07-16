/*
 * fdcan.c — STM32H753ZI FDCAN1 driver
 *
 * Mode   : Classic CAN, 1Mbit, Internal Loopback
 * Pins   : PA11=RX (AF9), PA12=TX (AF9)
 * Clock  : fdcan_ker_ck = HSE = 8MHz (FDCANSEL = 01)
 *           Nominal: 1Mbit @ 8MHz → prescaler=1, TSEG1=5, TSEG2=2, SJW=1
 *
 * Message RAM layout (FDCAN1):
 *   Rx FIFO 0 : 1 element × 18 words
 *   Tx Buffer : 1 element × 18 words
 *
 * REF: RM0433 Rev8 Chapter 56
 */

#include "fdcan.h"
#include <stdint.h>

/* ── RCC ── */
#define RCC_BASE       0x58024400UL
#define RCC_AHB4ENR    (*(volatile uint32_t *)(RCC_BASE + 0x0E0U))  /* GPIOAEN bit0 */
#define RCC_APB1HENR   (*(volatile uint32_t *)(RCC_BASE + 0x0ECU))  /* FDCANEN bit8 */
#define RCC_D2CCIP1R   (*(volatile uint32_t *)(RCC_BASE + 0x094U))  /* FDCANSEL bits[29:28] */

/* ── GPIOA ── */
#define GPIOA_BASE     0x58020000UL
#define GPIOA_MODER    (*(volatile uint32_t *)(GPIOA_BASE + 0x00U))
#define GPIOA_OSPEEDR  (*(volatile uint32_t *)(GPIOA_BASE + 0x08U))
#define GPIOA_PUPDR    (*(volatile uint32_t *)(GPIOA_BASE + 0x0CU))
#define GPIOA_AFRH     (*(volatile uint32_t *)(GPIOA_BASE + 0x24U))

/* ── FDCAN1 ── */
#define FDCAN1_BASE    0x4000A000UL
#define FDCAN1_CCCR    (*(volatile uint32_t *)(FDCAN1_BASE + 0x018U))
#define FDCAN1_NBTP    (*(volatile uint32_t *)(FDCAN1_BASE + 0x01CU))
#define FDCAN1_TEST    (*(volatile uint32_t *)(FDCAN1_BASE + 0x010U))
#define FDCAN1_RXF0C   (*(volatile uint32_t *)(FDCAN1_BASE + 0x0A0U))
#define FDCAN1_RXF0S   (*(volatile uint32_t *)(FDCAN1_BASE + 0x0A4U))
#define FDCAN1_RXF0A   (*(volatile uint32_t *)(FDCAN1_BASE + 0x0A8U))
#define FDCAN1_TXBC    (*(volatile uint32_t *)(FDCAN1_BASE + 0x0C0U))
#define FDCAN1_TXFQS   (*(volatile uint32_t *)(FDCAN1_BASE + 0x0C4U))
#define FDCAN1_TXBAR   (*(volatile uint32_t *)(FDCAN1_BASE + 0x0D0U))
#define FDCAN1_SIDFC   (*(volatile uint32_t *)(FDCAN1_BASE + 0x084U))
#define FDCAN1_GFC     (*(volatile uint32_t *)(FDCAN1_BASE + 0x080U))
#define FDCAN1_RXGFC   (*(volatile uint32_t *)(FDCAN1_BASE + 0x080U))

/* ── Message RAM ── */
/* FDCAN message RAM base — shared between FDCAN1 and FDCAN2 */
#define MSG_RAM_BASE   0x4000AC00UL

/* Layout in message RAM (word offsets from MSG_RAM_BASE):
 * 0x000 : Rx FIFO 0  — 1 element = 4 header words + 2 data words = 6 words
 * 0x018 : Tx Buffer  — 1 element = 4 header words + 2 data words = 6 words
 */
#define RX_FIFO0_OFFSET  0x000U   /* byte offset in message RAM */
#define TX_BUF_OFFSET    0x018U   /* byte offset in message RAM */

/* Message RAM element: 2 header words + up to 2 data words (8 bytes) */
typedef struct {
    volatile uint32_t hdr0;
    volatile uint32_t hdr1;
    volatile uint32_t data[2];
} fdcan_msg_t;

#define RX_FIFO0_ELEM  ((fdcan_msg_t *)(MSG_RAM_BASE + RX_FIFO0_OFFSET))
#define TX_BUF_ELEM    ((fdcan_msg_t *)(MSG_RAM_BASE + TX_BUF_OFFSET))

/* ── CCCR bits ── */
#define CCCR_INIT  (1UL << 0U)
#define CCCR_CCE   (1UL << 1U)
#define CCCR_TEST  (1UL << 7U)

/* ── TEST bits ── */
#define TEST_LBCK  (1UL << 4U)

/* ── Timeout ── */
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

/*
 * fdcan_init
 */
void fdcan_init(void)
{
    /* 1. Clocks */
    RCC_AHB4ENR  |= (1UL << 0U);   /* GPIOAEN */
    RCC_APB1HENR |= (1UL << 8U);   /* FDCANEN */

    /* FDCANSEL = 01 → HSE (8MHz) as fdcan_ker_ck */
    RCC_D2CCIP1R &= ~(3UL << 28U);
    RCC_D2CCIP1R |=  (1UL << 28U);

    /* 2. GPIO PA11=RX, PA12=TX → AF9, very high speed */
    GPIOA_MODER &= ~((3UL << (11U * 2U)) | (3UL << (12U * 2U)));
    GPIOA_MODER |=   (2UL << (11U * 2U)) | (2UL << (12U * 2U));
    GPIOA_OSPEEDR |= (3UL << (11U * 2U)) | (3UL << (12U * 2U));
    GPIOA_PUPDR  &= ~((3UL << (11U * 2U)) | (3UL << (12U * 2U)));
    /* AFRH: PA11=AF9 bits[15:12], PA12=AF9 bits[19:16] */
    GPIOA_AFRH &= ~((0xFUL << 12U) | (0xFUL << 16U));
    GPIOA_AFRH |=   (9UL   << 12U) | (9UL   << 16U);

    /* 3. Enter init+config mode */
    FDCAN1_CCCR |= CCCR_INIT;
    if (fdcan_wait_init(1) < 0) return;
    FDCAN1_CCCR |= CCCR_CCE;

    /* 4. Bit timing — 1Mbit @ 8MHz HSE
     *    Prescaler (NBRP+1)=1, NTSEG1=5, NTSEG2=2, NSJW=1
     *    Bit time = 1/8MHz * (1+5+2) = 1µs = 1Mbit ✓
     *    NBTP: [31:25]=NSJW, [24:16]=NTSEG1, [14:8]=NTSEG2, [8:0]=NBRP
     */
    FDCAN1_NBTP = ((0U  << 25U) |   /* NSJW = 0+1 = 1 */
                   (4U  << 16U) |   /* NTSEG1 = 4+1 = 5 */
                   (1U  <<  8U) |   /* NTSEG2 = 1+1 = 2 */
                   (0U  <<  0U));   /* NBRP = 0+1 = 1 */

    /* 5. Internal loopback */
    FDCAN1_CCCR |= CCCR_TEST;
    FDCAN1_TEST  = TEST_LBCK;

    /* 6. Message RAM — accept all frames, no filter */
    FDCAN1_GFC = 0x00000000U;   /* ANFS=00, ANFE=00 — store in Rx FIFO 0 */

    /* Rx FIFO 0: 1 element, start at offset 0 */
    FDCAN1_RXF0C = ((1U << 16U) |              /* F0S = 1 element */
                    ((RX_FIFO0_OFFSET / 4U) << 2U)); /* F0SA */

    /* Tx buffer: 1 dedicated buffer, start after Rx FIFO */
    FDCAN1_TXBC  = ((1U << 24U) |              /* NDTB = 1 */
                    ((TX_BUF_OFFSET / 4U) << 2U));   /* TBSA */

    /* 7. Leave init mode */
    FDCAN1_CCCR &= ~(CCCR_CCE | CCCR_INIT);
    fdcan_wait_init(0);
}

/* ─────────────────────────────────────────────────────────────
 * fdcan_transmit
 * ───────────────────────────────────────────────────────────── */
int8_t fdcan_transmit(const fdcan_frame_t *frame)
{
    /* Wait Tx buffer free */
    uint32_t t = FDCAN_TIMEOUT;
    while ((FDCAN1_TXFQS & (1UL << 21U)) && t--) {}  /* TFQF = full */
    if (!t) return -1;

    /* Write Tx buffer element */
    TX_BUF_ELEM->hdr0 = (frame->id & 0x7FFU) << 18U;  /* 11-bit ID */
    TX_BUF_ELEM->hdr1 = (frame->dlc & 0xFU) << 16U;   /* DLC */
    TX_BUF_ELEM->data[0] = ((uint32_t)frame->data[0]       |
                             ((uint32_t)frame->data[1] << 8U)  |
                             ((uint32_t)frame->data[2] << 16U) |
                             ((uint32_t)frame->data[3] << 24U));
    TX_BUF_ELEM->data[1] = ((uint32_t)frame->data[4]       |
                             ((uint32_t)frame->data[5] << 8U)  |
                             ((uint32_t)frame->data[6] << 16U) |
                             ((uint32_t)frame->data[7] << 24U));

    /* Request transmission of buffer 0 */
    FDCAN1_TXBAR = (1UL << 0U);

    return 0;
}

/*
 * fdcan_rx_pending
*/
uint8_t fdcan_rx_pending(void)
{
    return (uint8_t)((FDCAN1_RXF0S >> 0U) & 0x7FU);  /* F0FL: fill level */
}

/* 
 * fdcan_receive
 */
int8_t fdcan_receive(fdcan_frame_t *frame)
{
    if (!fdcan_rx_pending()) return -1;

    /* Read Rx FIFO 0 element */
    uint32_t get_idx = (FDCAN1_RXF0S >> 8U) & 0x3FU;  /* F0GI */
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

    /* Acknowledge — release FIFO slot */
    FDCAN1_RXF0A = get_idx;

    return 0;
}