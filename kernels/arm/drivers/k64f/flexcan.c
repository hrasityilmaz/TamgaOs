/*
 * flexcan.c — K64F FlexCAN0 driver
 *
 * Mode   : Classic CAN, 1Mbit
 * Pins   : PTB18=CAN0_TX (ALT2), 
 *          PTB19=CAN0_RX (ALT2)
 *          Verified against FRDM-K64F schematic — Arduino header D12/D11.
 * Clock  : bus clock = 60MHz (per mcg_init_120mhz: CLKDIV1 OUTDIV2=/2)
 *           CLKSRC=1 (peripheral/bus clock, not external oscillator)
 *           Sclock = 60MHz / (PRESDIV+1) = 60MHz / 5 = 12MHz
 *           Bit time = 12 quanta @ 12MHz = 1MHz ✓
 */

#include "flexcan.h"
#include "mmio_deviation.h" 
#include "uart.h"            
#include <stdint.h>

#define MB_TX_IDX (0U)
#define MB_RX_IDX (4U)

#define CAN_MB_CODE_TX_INACTIVE (0x8UL)
#define CAN_MB_CODE_TX_DATA     (0xCUL)
#define CAN_MB_CODE_RX_EMPTY    (0x4UL)

#define FLEXCAN_TIMEOUT (100000U)
#ifndef CAN_ESR1_FLTCONF_MASK
#define CAN_ESR1_FLTCONF_MASK (0x00000030UL)
#define CAN_ESR1_FLTCONF_SHIFT (4U)
#endif
#define CAN_ESR1_FLTCONF(x) (((uint32_t)(x) << CAN_ESR1_FLTCONF_SHIFT) & CAN_ESR1_FLTCONF_MASK)

static void uart_put_hex32(uint32_t v)
{
    static const char hex[] = "0123456789abcdef";
    char buf[11];
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 8; i++) {
        buf[2 + i] = hex[(v >> (28 - (i * 4))) & 0xFU];
    }
    buf[10] = '\0';
    uart_puts(buf);
}

static void uart_put_udec(uint32_t v)
{
    char buf[11]; 
    int i = 10;
    buf[i] = '\0';
    if (v == 0U) {
        buf[--i] = '0';
    } else {
        while (v != 0U && i > 0) {
            buf[--i] = (char)('0' + (v % 10U));
            v /= 10U;
        }
    }
    uart_puts(&buf[i]);
}

void flexcan_init(void)
{
    SIM->SCGC6 |= SIM_SCGC6_FLEXCAN0_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
    PORTB->PCR[18] = PORT_PCR_MUX(2U);
    PORTB->PCR[19] = PORT_PCR_MUX(2U); 
    FLEXCAN0->CTRL1 |= CAN_CTRL1_CLKSRC_MASK;
    FLEXCAN0->MCR &= ~CAN_MCR_MDIS_MASK;
    FLEXCAN0->MCR |= (CAN_MCR_FRZ_MASK | CAN_MCR_HALT_MASK);
    __asm volatile("dsb");
    __asm volatile("isb");

    {
        uint32_t t = FLEXCAN_TIMEOUT;
        while (((FLEXCAN0->MCR & CAN_MCR_FRZACK_MASK) == 0U) && (t != 0U)) { t--; }
    }

    FLEXCAN0->CTRL1 = (CAN_CTRL1_PRESDIV(4U) |
                       CAN_CTRL1_RJW(1U)     |
                       CAN_CTRL1_PSEG1(2U)   |
                       CAN_CTRL1_PSEG2(2U)   |
                       CAN_CTRL1_CLKSRC_MASK |
                       CAN_CTRL1_PROPSEG(4U));
                        /* CAN_CTRL1_LPB_MASK    |    LOOPBACK ENABLED — remove this line realworld !!!*/

    FLEXCAN0->RXMGMASK = 0x00000000UL;

    for (uint32_t i = 0U; i < 16U; i++) {
        FLEXCAN0->MB[i].CS = CAN_CS_CODE(CAN_MB_CODE_TX_INACTIVE);
    }

    FLEXCAN0->MB[MB_RX_IDX].ID = 0U;
    FLEXCAN0->MB[MB_RX_IDX].CS = CAN_CS_CODE(CAN_MB_CODE_RX_EMPTY);
    FLEXCAN0->MB[MB_TX_IDX].CS = CAN_CS_CODE(CAN_MB_CODE_TX_INACTIVE);
    FLEXCAN0->MCR &= ~CAN_MCR_HALT_MASK;
    {
        uint32_t t = FLEXCAN_TIMEOUT;
        while (((FLEXCAN0->MCR & CAN_MCR_FRZACK_MASK) != 0U) && (t != 0U)) { t--; }
    }
}

static void flexcan_recover_from_busoff(void)
{
    if ((FLEXCAN0->ESR1 & CAN_ESR1_FLTCONF_MASK) == CAN_ESR1_FLTCONF(2U)) {
        FLEXCAN0->MCR |= (CAN_MCR_FRZ_MASK | CAN_MCR_HALT_MASK);
        {
            uint32_t t = FLEXCAN_TIMEOUT;
            while (((FLEXCAN0->MCR & CAN_MCR_FRZACK_MASK) == 0U) && (t != 0U)) { t--; }
        }

        FLEXCAN0->MB[MB_TX_IDX].CS = CAN_CS_CODE(CAN_MB_CODE_TX_INACTIVE);

        FLEXCAN0->MCR &= ~CAN_MCR_HALT_MASK;
        {
            uint32_t t = FLEXCAN_TIMEOUT;
            while (((FLEXCAN0->MCR & CAN_MCR_FRZACK_MASK) != 0U) && (t != 0U)) { t--; }
        }
    }
}

int8_t flexcan_transmit(const flexcan_frame_t *frame)
{
    flexcan_recover_from_busoff();
    uint32_t t = FLEXCAN_TIMEOUT;
    while ((((FLEXCAN0->MB[MB_TX_IDX].CS & CAN_CS_CODE_MASK) >> CAN_CS_CODE_SHIFT)
            != CAN_MB_CODE_TX_INACTIVE) && (t != 0U)) { t--; }
    if (t == 0U) {
        //uart_puts("[K64F DEBUG] tx_ret=-1 (MB0 never went INACTIVE) MB0.CS=");
        uart_put_hex32(FLEXCAN0->MB[MB_TX_IDX].CS);
        uart_puts(" ESR1=");
        uart_put_hex32(FLEXCAN0->ESR1);
        uart_puts("\r\n");
        return -1;
    }

    FLEXCAN0->MB[MB_TX_IDX].ID = CAN_ID_STD(frame->id & 0x7FFU);
    FLEXCAN0->MB[MB_TX_IDX].WORD0 = ((uint32_t)frame->data[0] << 24U) |
                                     ((uint32_t)frame->data[1] << 16U) |
                                     ((uint32_t)frame->data[2] <<  8U) |
                                     ((uint32_t)frame->data[3] <<  0U);
    FLEXCAN0->MB[MB_TX_IDX].WORD1 = ((uint32_t)frame->data[4] << 24U) |
                                     ((uint32_t)frame->data[5] << 16U) |
                                     ((uint32_t)frame->data[6] <<  8U) |
                                     ((uint32_t)frame->data[7] <<  0U);

    FLEXCAN0->MB[MB_TX_IDX].CS = CAN_CS_CODE(CAN_MB_CODE_TX_DATA) |
                                  CAN_CS_SRR_MASK |
                                  CAN_CS_DLC(frame->dlc & 0xFU);

    t = FLEXCAN_TIMEOUT;
    while (((FLEXCAN0->IFLAG1 & (1UL << MB_TX_IDX)) == 0U) && (t != 0U)) { t--; }
    if (t == 0U) {
        uart_puts("[K64F DEBUG] tx_ret=-1 (IFLAG1 never set) MB0.CS=");
        uart_put_hex32(FLEXCAN0->MB[MB_TX_IDX].CS);
        uart_puts(" ESR1=");
        uart_put_hex32(FLEXCAN0->ESR1);
        uart_puts("\r\n");
        return -1;
    }

    FLEXCAN0->IFLAG1 = (1UL << MB_TX_IDX); 

    uart_puts("[K64F DEBUG] tx_ret=0 ID=");
    uart_put_udec(frame->id);
    uart_puts(" ESR1=");
    uart_put_hex32(FLEXCAN0->ESR1);
    uart_puts("\r\n");

    return 0;
}

uint8_t flexcan_rx_pending(void)
{
    return (uint8_t)((FLEXCAN0->IFLAG1 & (1UL << MB_RX_IDX)) != 0U);
}

int8_t flexcan_receive(flexcan_frame_t *frame)
{
    if (!flexcan_rx_pending()) return -1;
    uint32_t cs = FLEXCAN0->MB[MB_RX_IDX].CS; 
    frame->id  = (FLEXCAN0->MB[MB_RX_IDX].ID & CAN_ID_STD_MASK) >> CAN_ID_STD_SHIFT;
    frame->dlc = (uint8_t)((cs & CAN_CS_DLC_MASK) >> CAN_CS_DLC_SHIFT);

    uint32_t w0 = FLEXCAN0->MB[MB_RX_IDX].WORD0;
    uint32_t w1 = FLEXCAN0->MB[MB_RX_IDX].WORD1;
    frame->data[0] = (uint8_t)(w0 >> 24U);
    frame->data[1] = (uint8_t)(w0 >> 16U);
    frame->data[2] = (uint8_t)(w0 >>  8U);
    frame->data[3] = (uint8_t)(w0 >>  0U);
    frame->data[4] = (uint8_t)(w1 >> 24U);
    frame->data[5] = (uint8_t)(w1 >> 16U);
    frame->data[6] = (uint8_t)(w1 >>  8U);
    frame->data[7] = (uint8_t)(w1 >>  0U);
    (void)FLEXCAN0->TIMER;
    FLEXCAN0->IFLAG1 = (1UL << MB_RX_IDX); 

    return 0;
}