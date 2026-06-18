#ifndef MK64F12REGS_H
#define MK64F12REGS_H

/*
 * Minimal K64F Header for TamgaOs --MK64F12--
 * MISRA C:2012 register layout definitions
 */

#include <stdint.h>

/*
 * SIM Register Map (Base = 0x4004_7000)
 */
typedef struct {
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
  volatile uint32_t CLKDIV1;   /* 0x40048044 */
  volatile uint32_t CLKDIV2;   /* 0x40048048 */
  volatile uint32_t FCFG1;
  volatile uint32_t FCFG2;
  volatile uint32_t RESERVED4[2];
  volatile uint32_t FCFG3;
} SIM_Type;

/*
 * PORT Register Map
 */
typedef struct {
  volatile uint32_t PCR[32];
  volatile uint32_t GPCLR;
  volatile uint32_t GPCHR;
  volatile uint32_t RESERVED0[6];
  volatile uint32_t ISFR;
} PORT_Type;

/*
 * GPIO Register Map
 */
typedef struct {
  volatile uint32_t PDOR;
  volatile uint32_t PSOR;
  volatile uint32_t PCOR;
  volatile uint32_t PTOR;
  volatile uint32_t PDIR;
  volatile uint32_t PDDR;
} GPIO_Type;

typedef struct {
  volatile uint16_t STCTRLH;
  volatile uint16_t STCTRLL;
  volatile uint16_t TOVALH;
  volatile uint16_t TOVALL;
  volatile uint16_t WINH;
  volatile uint16_t WINL;
  volatile uint16_t REFRESH;
  volatile uint16_t UNLOCK;
} WDOG_Type;

typedef struct {
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

/*
 * ENET Register Map (Base = 0x400C0000)
 */
typedef struct {
    volatile uint32_t ECR;
    volatile uint32_t RESERVED0[1];
    volatile uint32_t MII_CTRL;
    volatile uint32_t MII_CMD; 
    volatile uint32_t MII_DATA;
    volatile uint32_t RESERVED1[0x3C];
    volatile uint32_t TCTRL;
    volatile uint32_t RESERVED2[0x3E];
    volatile uint32_t RCTRL;
} ENET_Type;

/*
 * SysTick Register Map
 */
typedef struct {
  volatile uint32_t CTRL;
  volatile uint32_t LOAD;
  volatile uint32_t VAL;
  volatile uint32_t CALIB;
} SysTick_Type;

/*
 * MCG Register Map (Base = 0x4006_4000)
 */
typedef struct {
  volatile uint8_t C1;
  volatile uint8_t C2;
  volatile uint8_t C3;
  volatile uint8_t C4;
  volatile uint8_t C5;
  volatile uint8_t C6;
  volatile uint8_t S;
  volatile uint8_t RESERVED1;
  volatile uint8_t ATCVH;
  volatile uint8_t ATCVL;
  volatile uint8_t C7;
  volatile uint8_t C8;
} MCG_Type;

//PIT TIMER
typedef struct {
    volatile uint32_t LDVAL;  
    volatile uint32_t CVAL;    
    volatile uint32_t TCTRL;  
    volatile uint32_t TFLG; 
} PIT_Channel_Type;

typedef struct {
    volatile uint32_t   MCR;          
    volatile uint32_t   RESERVED[63];
    PIT_Channel_Type    CHANNEL[4]; 
} PIT_Type;

#define PIT_MCR_MDIS_MASK  (1UL << 1U)
#define PIT_MCR_FRZ_MASK   (1UL << 0U)
#define PIT_TCTRL_TEN_MASK (1UL << 0U)
#define PIT_TCTRL_TIE_MASK (1UL << 1U)
#define PIT_TFLG_TIF_MASK  (1UL << 0U)
#define SIM_SCGC6_PIT_MASK (1UL << 23U)
#define PIT0_IRQ_NUMBER    (48U)

/*
*  MCG Bit Definitions
*/
 

#define MCG_C1_IREFSTEN_MASK (0x01U)
#define MCG_C1_IRCLKEN_MASK (0x02U)
#define MCG_C1_IREFS_MASK (0x04U)
#define MCG_C1_FRDIV_SHIFT (3U)
#define MCG_C1_FRDIV_WIDTH (3U)
#define MCG_C1_FRDIV_MASK                                                      \
  (((1UL << MCG_C1_FRDIV_WIDTH) - 1UL) << MCG_C1_FRDIV_SHIFT)
#define MCG_C1_FRDIV(x)                                                        \
  (((uint8_t)(x) << MCG_C1_FRDIV_SHIFT) & MCG_C1_FRDIV_MASK)
#define MCG_C1_CLKS_SHIFT (6U)
#define MCG_C1_CLKS_WIDTH (2U)
#define MCG_C1_CLKS_MASK                                                       \
  (((1UL << MCG_C1_CLKS_WIDTH) - 1UL) << MCG_C1_CLKS_SHIFT)
#define MCG_C1_CLKS(x) (((uint8_t)(x) << MCG_C1_CLKS_SHIFT) & MCG_C1_CLKS_MASK)

#define MCG_C2_IRCS_MASK (0x01U)
#define MCG_C2_LP_MASK (0x02U)
#define MCG_C2_EREFS0_MASK (0x04U)
#define MCG_C2_HGO0_MASK (0x08U)
#define MCG_C2_RANGE0_SHIFT (4U)
#define MCG_C2_RANGE0_WIDTH (2U)
#define MCG_C2_RANGE0_MASK                                                     \
  (((1UL << MCG_C2_RANGE0_WIDTH) - 1UL) << MCG_C2_RANGE0_SHIFT)
#define MCG_C2_RANGE0(x)                                                       \
  (((uint8_t)(x) << MCG_C2_RANGE0_SHIFT) & MCG_C2_RANGE0_MASK)
#define MCG_C2_LOCRE0_MASK (0x80U)

#define MCG_C5_PRDIV0_SHIFT (0U)
#define MCG_C5_PRDIV0_WIDTH (5U)
#define MCG_C5_PRDIV0_MASK                                                     \
  (((1UL << MCG_C5_PRDIV0_WIDTH) - 1UL) << MCG_C5_PRDIV0_SHIFT)
#define MCG_C5_PRDIV0(x)                                                       \
  (((uint8_t)(x) << MCG_C5_PRDIV0_SHIFT) & MCG_C5_PRDIV0_MASK)
#define MCG_C5_PLLCLKEN0_MASK (0x40U)

#define MCG_C6_VDIV0_SHIFT (0U)
#define MCG_C6_VDIV0_WIDTH (5U)
#define MCG_C6_VDIV0_MASK                                                      \
  (((1UL << MCG_C6_VDIV0_WIDTH) - 1UL) << MCG_C6_VDIV0_SHIFT)
#define MCG_C6_VDIV0(x)                                                        \
  (((uint8_t)(x) << MCG_C6_VDIV0_SHIFT) & MCG_C6_VDIV0_MASK)
#define MCG_C6_CME0_MASK (0x20U)
#define MCG_C6_PLLS_MASK (0x40U)
#define MCG_C6_LOLIE0_MASK (0x80U)

#define MCG_S_IRCST_MASK (0x01U)
#define MCG_S_OSCINIT0_MASK (0x02U)
#define MCG_S_CLKST_SHIFT (2U)
#define MCG_S_CLKST_WIDTH (2U)
#define MCG_S_CLKST_MASK                                                       \
  (((1UL << MCG_S_CLKST_WIDTH) - 1UL) << MCG_S_CLKST_SHIFT)
#define MCG_S_IREFST_MASK (0x10U)
#define MCG_S_PLLST_MASK (0x20U)
#define MCG_S_LOCK0_MASK (0x40U)
#define MCG_S_LOLS0_MASK (0x80U)

#define MCG_CLKS_FLL_PLL (0x0U)
#define MCG_CLKS_INTERNAL (0x1U)
#define MCG_CLKS_EXTERNAL (0x2U)

/* ============================================================
 * SIM Bit Definitions
 * ============================================================ */
#define SIM_CLKDIV1_OUTDIV4_SHIFT (16U)
#define SIM_CLKDIV1_OUTDIV3_SHIFT (20U)
#define SIM_CLKDIV1_OUTDIV2_SHIFT (24U)
#define SIM_CLKDIV1_OUTDIV1_SHIFT (28U)

#define SIM_CLKDIV1_OUTDIV4(x) (((uint32_t)(x) & 0xFUL) << SIM_CLKDIV1_OUTDIV4_SHIFT)
#define SIM_CLKDIV1_OUTDIV3(x) (((uint32_t)(x) & 0xFUL) << SIM_CLKDIV1_OUTDIV3_SHIFT)
#define SIM_CLKDIV1_OUTDIV2(x) (((uint32_t)(x) & 0xFUL) << SIM_CLKDIV1_OUTDIV2_SHIFT)
#define SIM_CLKDIV1_OUTDIV1(x) (((uint32_t)(x) & 0xFUL) << SIM_CLKDIV1_OUTDIV1_SHIFT)

#define SIM_SCGC2_ENET_MASK  (1UL << 1U)   /* ENET clock gating */

#define SIM_SCGC4_UART0_MASK (1UL << 10U)  /* UART0 clock */

/* SIM_SCGC5 Port Clock Gates - K64F RM Table 12-2 */
#define SIM_SCGC5_PORTA_MASK (1UL << 9U)
#define SIM_SCGC5_PORTB_MASK (1UL << 10U)
#define SIM_SCGC5_PORTC_MASK (1UL << 11U)
#define SIM_SCGC5_PORTD_MASK (1UL << 12U)
#define SIM_SCGC5_PORTE_MASK (1UL << 13U)

#define SIM_SOPT2_PLLFLLSEL_SHIFT (28U)
#define SIM_SOPT2_PLLFLLSEL_WIDTH (1U)
#define SIM_SOPT2_PLLFLLSEL_MASK                                               \
  (((1UL << SIM_SOPT2_PLLFLLSEL_WIDTH) - 1UL) << SIM_SOPT2_PLLFLLSEL_SHIFT)
#define SIM_SOPT2_PLLFLLSEL(x)                                                 \
  (((uint32_t)(x) << SIM_SOPT2_PLLFLLSEL_SHIFT) & SIM_SOPT2_PLLFLLSEL_MASK)
#define SIM_SOPT2_PLLFLLSEL_MCGFLLCLK (0x0U)
#define SIM_SOPT2_PLLFLLSEL_MCGPLLCLK (0x1U)

#define SIM_SOPT2_UART0SRC_SHIFT (26U)
#define SIM_SOPT2_UART0SRC_WIDTH (2U)
#define SIM_SOPT2_UART0SRC_MASK                                                \
  (((1UL << SIM_SOPT2_UART0SRC_WIDTH) - 1UL) << SIM_SOPT2_UART0SRC_SHIFT)
#define SIM_SOPT2_UART0SRC(x)                                                  \
  (((uint32_t)(x) << SIM_SOPT2_UART0SRC_SHIFT) & SIM_SOPT2_UART0SRC_MASK)
#define SIM_SOPT2_UART0SRC_DISABLED (0x0U)
#define SIM_SOPT2_UART0SRC_MCGFLL (0x1U)
#define SIM_SOPT2_UART0SRC_OSCERCLK (0x2U)
#define SIM_SOPT2_UART0SRC_MCGIRCLK (0x3U)

/* ============================================================
 * ENET Bit Definitions
 * ============================================================ */
#define ENET_ECR_RESET_MASK (1UL << 0U)
#define ENET_ECR_ENABLE_MASK (1UL << 1U)

#define ENET_RCTRL_MII_MODE_MASK (1UL << 1U)

/* ============================================================
 * UART Bit Definitions
 * ============================================================ */
#define UART_C2_TE_MASK (0x08U)
#define UART_C2_RE_MASK (0x04U)
#define UART_S1_TDRE_MASK (0x80U)
#define UART_S1_RDRF_MASK (0x20U)

#define UART_BDH_SBR_SHIFT (0U)
#define UART_BDH_SBR_WIDTH (5U)
#define UART_BDH_SBR_MASK                                                      \
  (((1UL << UART_BDH_SBR_WIDTH) - 1UL) << UART_BDH_SBR_SHIFT)

#define UART_C4_BRFA_SHIFT (0U)
#define UART_C4_BRFA_WIDTH (5U)
#define UART_C4_BRFA_MASK                                                      \
  (((1UL << UART_C4_BRFA_WIDTH) - 1UL) << UART_C4_BRFA_SHIFT)

/* ============================================================
 * WDOG Bit Definitions
 * ============================================================ */
#define WDOG_UNLOCK_KEY1 (0xC520U)
#define WDOG_UNLOCK_KEY2 (0xD928U)
#define WDOG_STCTRLH_DISABLE (0x01D2U)

/* ============================================================
 * SysTick Bit Definitions
 * ============================================================ */
#define SYSTICK_CTRL_ENABLE_SHIFT (0U)
#define SYSTICK_CTRL_ENABLE_MASK (1UL << SYSTICK_CTRL_ENABLE_SHIFT)
#define SYSTICK_CTRL_TICKINT_SHIFT (1U)
#define SYSTICK_CTRL_TICKINT_MASK (1UL << SYSTICK_CTRL_TICKINT_SHIFT)
#define SYSTICK_CTRL_CLKSOURCE_SHIFT (2U)
#define SYSTICK_CTRL_CLKSOURCE_MASK (1UL << SYSTICK_CTRL_CLKSOURCE_SHIFT)
#define SYSTICK_CTRL_COUNTFLAG_SHIFT (16U)
#define SYSTICK_CTRL_COUNTFLAG_MASK (1UL << SYSTICK_CTRL_COUNTFLAG_SHIFT)

#define SYSTICK_LOAD_RELOAD_MASK (0x00FFFFFFUL)

/* ============================================================
 * PORT Bit Definitions
 * ============================================================ */
#define PORT_PCR_MUX_SHIFT (8U)
#define PORT_PCR_MUX_WIDTH (3U)
#define PORT_PCR_MUX_MASK                                                      \
  (((1UL << PORT_PCR_MUX_WIDTH) - 1UL) << PORT_PCR_MUX_SHIFT)
#define PORT_PCR_MUX(x)                                                        \
  (((uint32_t)(x) << PORT_PCR_MUX_SHIFT) & PORT_PCR_MUX_MASK)

#endif