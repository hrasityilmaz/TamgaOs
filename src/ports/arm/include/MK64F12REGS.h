#ifndef MK64F12REGS_H
#define MK64F12REGS_H

/*
 * Minimal K64F Header for TamgaOs --MK64F12--
 * MISRA C:2012 register layout definitions
 */

#include <stdint.h>


/*
 * SIM Register Map (Base = 0x4004_7000)
 * -----------------------------------------------------------------
 * 0x4004_7000  SOPT1     System Options Register 1          32 R/W
 * 0x4004_7004  SOPT1CFG  SOPT1 Configuration Register       32 R/W
 * 0x4004_8004  SOPT2     System Options Register 2          32 R/W
 * 0x4004_800C  SOPT4     System Options Register 4          32 R/W
 * 0x4004_8010  SOPT5     System Options Register 5          32 R/W
 * 0x4004_8018  SOPT7     System Options Register 7          32 R/W
 * 0x4004_8024  SDID      System Device Identification Reg   32 R
 * 0x4004_8028  SCGC1     System Clock Gating Control Reg 1  32 R/W
 * 0x4004_802C  SCGC2     System Clock Gating Control Reg 2  32 R/W
 * 0x4004_8030  SCGC3     System Clock Gating Control Reg 3  32 R/W
 * 0x4004_8034  SCGC4     System Clock Gating Control Reg 4  32 R/W
 * 0x4004_8038  SCGC5     System Clock Gating Control Reg 5  32 R/W
 * 0x4004_803C  SCGC6     System Clock Gating Control Reg 6  32 R/W
 * 0x4004_8040  SCGC7     System Clock Gating Control Reg 7  32 R/W
 * -----------------------------------------------------------------
 */
typedef struct
{
    volatile uint32_t SOPT1;
    volatile uint32_t SOPT1CFG;
    volatile uint32_t RESERVED0[1023];
    volatile uint32_t SOPT2;
    volatile uint32_t RESERVED1[1];
    volatile uint32_t SOPT4;
    volatile uint32_t SOPT5;
    volatile uint32_t RESERVED2[1];
    volatile uint32_t SOPT7;
    volatile uint32_t RESERVED3[2];
    volatile uint32_t SDID;
    volatile uint32_t SCGC1;
    volatile uint32_t SCGC2;
    volatile uint32_t SCGC3;
    volatile uint32_t SCGC4;
    volatile uint32_t SCGC5;
    volatile uint32_t SCGC6;
    volatile uint32_t SCGC7;
} SIM_Type;

/*
 * PORT Register Map
 * -----------------------------------------------------------------
 * PCR[0..31]   Pin Control Registers
 * GPCLR        Global Pin Control Low Register
 * GPCHR        Global Pin Control High Register
 * ISFR         Interrupt Status Flag Register
 * -----------------------------------------------------------------
 */
typedef struct
{
    volatile uint32_t PCR[32];
    volatile uint32_t GPCLR;
    volatile uint32_t GPCHR;
    volatile uint32_t RESERVED0[6];
    volatile uint32_t ISFR;
} PORT_Type;

/*
 * GPIO Register Map
 * -----------------------------------------------------------------
 * PDOR  Port Data Output Register
 * PSOR  Port Set Output Register
 * PCOR  Port Clear Output Register
 * PTOR  Port Toggle Output Register
 * PDIR  Port Data Input Register
 * PDDR  Port Data Direction Register
 * -----------------------------------------------------------------
 */
typedef struct
{
    volatile uint32_t PDOR;
    volatile uint32_t PSOR;
    volatile uint32_t PCOR;
    volatile uint32_t PTOR;
    volatile uint32_t PDIR;
    volatile uint32_t PDDR;
} GPIO_Type;

typedef struct
{
    volatile uint16_t STCTRLH;
    volatile uint16_t STCTRLL;
    volatile uint16_t TOVALH;
    volatile uint16_t TOVALL;
    volatile uint16_t WINH;
    volatile uint16_t WINL;
    volatile uint16_t REFRESH;
    volatile uint16_t UNLOCK;
} WDOG_Type;

typedef struct
{
    volatile uint8_t BDH;
    volatile uint8_t BDL;
    volatile uint8_t C1;
    volatile uint8_t C2;
    volatile uint8_t S1;
    volatile uint8_t S2;
    volatile uint8_t C3;
    volatile uint8_t D;
    volatile uint8_t MA1;
    volatile uint8_t MA2;
    volatile uint8_t C4;
    volatile uint8_t C5;
} UART_Type;


#define WDOG_UNLOCK_KEY1       (0xC520U)
#define WDOG_UNLOCK_KEY2       (0xD928U)
#define WDOG_STCTRLH_DISABLE   (0x01D2U)

#define UART_C2_TE_MASK        (0x08U)
#define UART_C2_RE_MASK        (0x04U)
#define UART_S1_TDRE_MASK      (0x80U)
#define UART_S1_RDRF_MASK      (0x20U)

#define PORT_PCR_MUX_SHIFT     (8U)
#define PORT_PCR_MUX_WIDTH     (3U)
#define PORT_PCR_MUX_MASK      (((1UL << PORT_PCR_MUX_WIDTH) - 1UL) << PORT_PCR_MUX_SHIFT)

#define PORT_PCR_MUX(x) (((uint32_t)(x) << PORT_PCR_MUX_SHIFT) & PORT_PCR_MUX_MASK)


/* PLLFLLSEL: bit 28 */
#define SIM_SOPT2_PLLFLLSEL_SHIFT     (28U)
#define SIM_SOPT2_PLLFLLSEL_WIDTH     (1U)
#define SIM_SOPT2_PLLFLLSEL_MASK      (((1UL << SIM_SOPT2_PLLFLLSEL_WIDTH) - 1UL) << SIM_SOPT2_PLLFLLSEL_SHIFT)

#define SIM_SOPT2_PLLFLLSEL(x) (((uint32_t)(x) << SIM_SOPT2_PLLFLLSEL_SHIFT) & SIM_SOPT2_PLLFLLSEL_MASK)

#define SIM_SOPT2_PLLFLLSEL_MCGFLLCLK  (0x0U)
#define SIM_SOPT2_PLLFLLSEL_MCGPLLCLK  (0x1U)

/* UART0SRC: bits 26 27 */
#define SIM_SOPT2_UART0SRC_SHIFT       (26U)
#define SIM_SOPT2_UART0SRC_WIDTH       (2U)
#define SIM_SOPT2_UART0SRC_MASK        (((1UL << SIM_SOPT2_UART0SRC_WIDTH) - 1UL) << SIM_SOPT2_UART0SRC_SHIFT)

#define SIM_SOPT2_UART0SRC(x) (((uint32_t)(x) << SIM_SOPT2_UART0SRC_SHIFT) & SIM_SOPT2_UART0SRC_MASK)

#define SIM_SOPT2_UART0SRC_DISABLED    (0x0U)
#define SIM_SOPT2_UART0SRC_MCGFLL      (0x1U)
#define SIM_SOPT2_UART0SRC_OSCERCLK    (0x2U)
#define SIM_SOPT2_UART0SRC_MCGIRCLK    (0x3U)

#define SIM_SCGC4_UART0_SHIFT  (10U)
#define SIM_SCGC4_UART0_MASK   (1UL << SIM_SCGC4_UART0_SHIFT)

#define SIM_SCGC5_PORTB_SHIFT  (10U)
#define SIM_SCGC5_PORTB_MASK   (1UL << SIM_SCGC5_PORTB_SHIFT)

#define UART_BDH_SBR_SHIFT     (0U)
#define UART_BDH_SBR_WIDTH     (5U)
#define UART_BDH_SBR_MASK      (((1UL << UART_BDH_SBR_WIDTH) - 1UL) << UART_BDH_SBR_SHIFT)


#endif