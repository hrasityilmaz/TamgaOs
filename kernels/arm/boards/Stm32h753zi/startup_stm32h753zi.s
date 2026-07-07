.syntax unified
.cpu    cortex-m7
.thumb

/*
 * startup_stm32h753zi.s
 * REF: RM0433 Rev8 Table 145 — NVIC vector table
 * REF: PM0253 Rev6 Section 4.8 — Cache enabling sequence
 *
 * Reset sequence:
 *   1. Invalidate I-Cache (ICIALLU)
 *   2. Invalidate D-Cache (DCISW set/way loop)
 *   3. Enable I-Cache + D-Cache together (CCR.IC | CCR.DC)
 *   4. Copy .data  Flash → DTCM
 *   5. Zero  .bss  in DTCM
 *   6. Call main
 *
 * PM0253: "L1 data and instruction cache must be invalidated before they
 *          are enabled, otherwise unpredictable behavior can occur."
 */

/* ─────────────────────────────────────────────────────────────────────────
 * Vector table — 16 system exceptions + 150 IRQs = 166 entries
 * REF: RM0433 Table 145
 * ───────────────────────────────────────────────────────────────────────── */
.section .vectors, "a", %progbits
.align 10
_vectors:
    /* -- Cortex-M system exceptions -- */
    .word  _stack_top               /* 0  Initial SP */
    .word  Reset_Handler            /* 1  Reset */
    .word  Default_Handler          /* 2  NMI */
    .word  HardFault_Handler        /* 3  HardFault */
    .word  Default_Handler          /* 4  MemManage */
    .word  Default_Handler          /* 5  BusFault */
    .word  Default_Handler          /* 6  UsageFault */
    .word  0                        /* 7  reserved */
    .word  0                        /* 8  reserved */
    .word  0                        /* 9  reserved */
    .word  0                        /* 10 reserved */
    .word  Default_Handler          /* 11 SVCall */
    .word  Default_Handler          /* 12 DebugMonitor */
    .word  0                        /* 13 reserved */
    .word  PendSV_Handler           /* 14 PendSV */
    .word  SysTick_Handler          /* 15 SysTick */

    /* -- IRQs (REF: RM0433 Table 145) -- */
    .word  Default_Handler          /* 0   WWDG1 */
    .word  Default_Handler          /* 1   PVD_PVM */
    .word  Default_Handler          /* 2   RTC_TAMP_STAMP_CSS_LSE */
    .word  Default_Handler          /* 3   RTC_WKUP */
    .word  Default_Handler          /* 4   FLASH */
    .word  Default_Handler          /* 5   RCC */
    .word  Default_Handler          /* 6   EXTI0 */
    .word  Default_Handler          /* 7   EXTI1 */
    .word  Default_Handler          /* 8   EXTI2 */
    .word  Default_Handler          /* 9   EXTI3 */
    .word  Default_Handler          /* 10  EXTI4 */
    .word  Default_Handler          /* 11  DMA_STR0 */
    .word  Default_Handler          /* 12  DMA_STR1 */
    .word  Default_Handler          /* 13  DMA_STR2 */
    .word  Default_Handler          /* 14  DMA_STR3 */
    .word  Default_Handler          /* 15  DMA_STR4 */
    .word  Default_Handler          /* 16  DMA_STR5 */
    .word  Default_Handler          /* 17  DMA_STR6 */
    .word  Default_Handler          /* 18  ADC1_2 */
    .word  Default_Handler          /* 19  FDCAN1_IT0 */
    .word  Default_Handler          /* 20  FDCAN2_IT0 */
    .word  Default_Handler          /* 21  FDCAN1_IT1 */
    .word  Default_Handler          /* 22  FDCAN2_IT1 */
    .word  Default_Handler          /* 23  EXTI9_5 */
    .word  Default_Handler          /* 24  TIM1_BRK */
    .word  Default_Handler          /* 25  TIM1_UP */
    .word  Default_Handler          /* 26  TIM1_TRG_COM */
    .word  Default_Handler          /* 27  TIM_CC */
    .word  Default_Handler          /* 28  TIM2 */
    .word  Default_Handler          /* 29  TIM3 */
    .word  Default_Handler          /* 30  TIM4 */
    .word  Default_Handler          /* 31  I2C1_EV */
    .word  Default_Handler          /* 32  I2C1_ER */
    .word  Default_Handler          /* 33  I2C2_EV */
    .word  Default_Handler          /* 34  I2C2_ER */
    .word  Default_Handler          /* 35  SPI1 */
    .word  Default_Handler          /* 36  SPI2 */
    .word  Default_Handler          /* 37  USART1 */
    .word  Default_Handler          /* 38  USART2 */
    .word  USART3_IRQHandler        /* 39  USART3 — Nucleo ST-Link UART (PD8/PD9) */
    .word  Default_Handler          /* 40  EXTI15_10 */
    .word  Default_Handler          /* 41  RTC_ALARM */
    .word  0                        /* 42  reserved */
    .word  Default_Handler          /* 43  TIM8_BRK_TIM12 */
    .word  Default_Handler          /* 44  TIM8_UP_TIM13 */
    .word  Default_Handler          /* 45  TIM8_TRG_COM_TIM14 */
    .word  Default_Handler          /* 46  TIM8_CC */
    .word  Default_Handler          /* 47  DMA1_STR7 */
    .word  Default_Handler          /* 48  FMC */
    .word  Default_Handler          /* 49  SDMMC1 */
    .word  Default_Handler          /* 50  TIM5 */
    .word  Default_Handler          /* 51  SPI3 */
    .word  Default_Handler          /* 52  UART4 */
    .word  Default_Handler          /* 53  UART5 */
    .word  Default_Handler          /* 54  TIM6_DAC */
    .word  TIM7_IRQHandler          /* 55  TIM7 — scheduler tick */
    .word  Default_Handler          /* 56  DMA2_STR0 */
    .word  Default_Handler          /* 57  DMA2_STR1 */
    .word  Default_Handler          /* 58  DMA2_STR2 */
    .word  Default_Handler          /* 59  DMA2_STR3 */
    .word  Default_Handler          /* 60  DMA2_STR4 */
    .word  Default_Handler          /* 61  ETH */
    .word  Default_Handler          /* 62  ETH_WKUP */
    .word  Default_Handler          /* 63  FDCAN_CAL */
    .word  Default_Handler          /* 64  Cortex-M7 Send Event */
    .word  0                        /* 65  NC */
    .word  0                        /* 66  NC */
    .word  0                        /* 67  NC */
    .word  Default_Handler          /* 68  DMA2_STR5 */
    .word  Default_Handler          /* 69  DMA2_STR6 */
    .word  Default_Handler          /* 70  DMA2_STR7 */
    .word  Default_Handler          /* 71  USART6 */
    .word  Default_Handler          /* 72  I2C3_EV */
    .word  Default_Handler          /* 73  I2C3_ER */
    .word  Default_Handler          /* 74  OTG_HS_EP1_OUT */
    .word  Default_Handler          /* 75  OTG_HS_EP1_IN */
    .word  Default_Handler          /* 76  OTG_HS_WKUP */
    .word  Default_Handler          /* 77  OTG_HS */
    .word  Default_Handler          /* 78  DCMI */
    .word  Default_Handler          /* 79  CRYP */
    .word  Default_Handler          /* 80  HASH_RNG */
    .word  Default_Handler          /* 81  FPU */
    .word  Default_Handler          /* 82  UART7 */
    .word  Default_Handler          /* 83  UART8 */
    .word  Default_Handler          /* 84  SPI4 */
    .word  Default_Handler          /* 85  SPI5 */
    .word  Default_Handler          /* 86  SPI6 */
    .word  Default_Handler          /* 87  SAI1 */
    .word  Default_Handler          /* 88  LTDC */
    .word  Default_Handler          /* 89  LTDC_ER */
    .word  Default_Handler          /* 90  DMA2D */
    .word  Default_Handler          /* 91  SAI2 */
    .word  Default_Handler          /* 92  QUADSPI */
    .word  Default_Handler          /* 93  LPTIM1 */
    .word  Default_Handler          /* 94  CEC */
    .word  Default_Handler          /* 95  I2C4_EV */
    .word  Default_Handler          /* 96  I2C4_ER */
    .word  Default_Handler          /* 97  SPDIF */
    .word  Default_Handler          /* 98  OTG_FS_EP1_OUT */
    .word  Default_Handler          /* 99  OTG_FS_EP1_IN */
    .word  Default_Handler          /* 100 OTG_FS_WKUP */
    .word  Default_Handler          /* 101 OTG_FS */
    .word  Default_Handler          /* 102 DMAMUX1_OV */
    .word  Default_Handler          /* 103 HRTIM1_MST */
    .word  Default_Handler          /* 104 HRTIM1_TIMA */
    .word  Default_Handler          /* 105 HRTIM_TIMB */
    .word  Default_Handler          /* 106 HRTIM1_TIMC */
    .word  Default_Handler          /* 107 HRTIM1_TIMD */
    .word  Default_Handler          /* 108 HRTIM_TIME */
    .word  Default_Handler          /* 109 HRTIM1_FLT */
    .word  Default_Handler          /* 110 DFSDM1_FLT0 */
    .word  Default_Handler          /* 111 DFSDM1_FLT1 */
    .word  Default_Handler          /* 112 DFSDM1_FLT2 */
    .word  Default_Handler          /* 113 DFSDM1_FLT3 */
    .word  Default_Handler          /* 114 SAI3 */
    .word  Default_Handler          /* 115 SWPMI1 */
    .word  Default_Handler          /* 116 TIM15 */
    .word  Default_Handler          /* 117 TIM16 */
    .word  Default_Handler          /* 118 TIM17 */
    .word  Default_Handler          /* 119 MDIOS_WKUP */
    .word  Default_Handler          /* 120 MDIOS */
    .word  Default_Handler          /* 121 JPEG */
    .word  Default_Handler          /* 122 MDMA */
    .word  0                        /* 123 reserved */
    .word  Default_Handler          /* 124 SDMMC2 */
    .word  Default_Handler          /* 125 HSEM0 */
    .word  0                        /* 126 reserved */
    .word  Default_Handler          /* 127 ADC3 */
    .word  Default_Handler          /* 128 DMAMUX2_OVR */
    .word  Default_Handler          /* 129 BDMA_CH0 */
    .word  Default_Handler          /* 130 BDMA_CH1 */
    .word  Default_Handler          /* 131 BDMA_CH2 */
    .word  Default_Handler          /* 132 BDMA_CH3 */
    .word  Default_Handler          /* 133 BDMA_CH4 */
    .word  Default_Handler          /* 134 BDMA_CH5 */
    .word  Default_Handler          /* 135 BDMA_CH6 */
    .word  Default_Handler          /* 136 BDMA_CH7 */
    .word  Default_Handler          /* 137 COMP */
    .word  Default_Handler          /* 138 LPTIM2 */
    .word  Default_Handler          /* 139 LPTIM3 */
    .word  Default_Handler          /* 140 LPTIM4 */
    .word  Default_Handler          /* 141 LPTIM5 */
    .word  Default_Handler          /* 142 LPUART */
    .word  Default_Handler          /* 143 WWDG1_RST */
    .word  Default_Handler          /* 144 CRS */
    .word  Default_Handler          /* 145 RAMECC */
    .word  Default_Handler          /* 146 SAI4 */
    .word  0                        /* 147 reserved */
    .word  0                        /* 148 reserved */
    .word  Default_Handler          /* 149 WKUP */

/* ─────────────────────────────────────────────────────────────────────────
 * Cache addresses MUST CONTROL !!!  (PM0253 Section 4.8)
 * ───────────────────────────────────────────────────────────────────────── */
.equ ICIALLU,  0xE000EF50   /* I-Cache invalidate all */
.equ CSSELR,   0xE000ED84   /* Cache size selection */
.equ CCSIDR,   0xE000ED80   /* Cache size ID */
.equ DCISW,    0xE000EF60   /* D-Cache invalidate by set/way */
.equ CCR,      0xE000ED14   /* Configuration and control register */

/* ─────────────────────────────────────────────────────────────────────────
 * Reset Handler
 * REF: PM0253 Section 4.8 — cache must be invalidated before enabling
 * ───────────────────────────────────────────────────────────────────────── */
.section .text, "ax", %progbits
.thumb_func
.global Reset_Handler
Reset_Handler:

    /* ── 0. ExitRun0Mode — must be done before writing to RAM
     *    STM32H7 starts in Run* mode where RAM writes are forbidden
     *    until PWR supply is configured. LDO supply: set LDOEN (bit1)
     *    in PWR_CR3 and wait for ACTVOSRDY (bit13) in PWR_CSR1.
     *    REF: RM0433 Section 6.4 (power-up sequence)
     * ── */
    ldr  r1, =0x58024800       @ PWR base
    ldr  r0, [r1, #0x00C]      @ PWR_CR3 offset 0x00C
    orr  r0, r0, #(1 << 1)     @ LDOEN bit1
    str  r0, [r1, #0x00C]
.Lwait_actvosrdy:
    ldr  r0, [r1, #0x004]      @ PWR_CSR1 offset 0x004
    tst  r0, #(1 << 13)        @ ACTVOSRDY bit13
    beq  .Lwait_actvosrdy

    /* ── 1. Invalidate I-Cache (PM0253: write 0 to ICIALLU) ── */
    /* page 243 Invalidate cache code  */
    ldr  r11, =ICIALLU
    mov  r0,  #0
    str  r0,  [r11]
    dsb
    isb

    /* ── 2. Invalidate D-Cache by set/way (PM0253 Section 4.8) ── */
    mov  r0,  #0
    ldr  r11, =CSSELR
    str  r0,  [r11]             /* select D-Cache level 0 */
    dsb

    ldr  r11, =CCSIDR
    ldr  r2,  [r11]             /* read cache geometry */
    and  r1,  r2, #0x7          /* line size encoding */
    add  r7,  r1, #0x4          /* log2(line size in words) + 2 */
    mov  r1,  #0x3ff
    ands r4,  r1, r2, lsr #3    /* number of ways - 1 */
    mov  r1,  #0x7fff
    ands r2,  r1, r2, lsr #13   /* number of sets - 1 */
    clz  r6,  r4                 /* bit position of way field */
    ldr  r11, =DCISW

.Linv_loop1:
    mov  r1,  r4
.Linv_loop2:
    lsl  r3,  r1, r6
    lsl  r8,  r2, r7
    orr  r3,  r3, r8
    str  r3,  [r11]             /* DCISW — invalidate line */
    subs r1,  r1, #1
    bge  .Linv_loop2
    subs r2,  r2, #1
    bge  .Linv_loop1
    dsb
    isb

    /* ── 3. Enable I-Cache + D-Cache together (PM0253: CCR.IC | CCR.DC) ── */
    ldr  r11, =CCR
    ldr  r0,  [r11]
    orr  r0,  r0, #(1 << 16)   /* DC bit */
    orr  r0,  r0, #(1 << 17)   /* IC bit */
    str  r0,  [r11]
    dsb
    isb

     /* ── 3.5. FPU enable — CPACR CP10/CP11 full access ── */
    ldr  r0, =0xE000ED88
    ldr  r1, [r0]
    orr  r1, r1, #(0xF << 20)
    str  r1, [r0]
    dsb
    isb
    
    /* ── 4. Copy .data: Flash → DTCM ── */
    ldr  r0, =_data_start
    ldr  r1, =_data_end
    ldr  r2, =_data_load
    cmp  r0, r1
    beq  .Lbss_init
.Lcopy_loop:
    ldr  r3, [r2], #4
    str  r3, [r0], #4
    cmp  r0, r1
    blt  .Lcopy_loop

    /* ── 5. Zero .bss in DTCM ── */
.Lbss_init:
    ldr  r0, =_bss_start
    ldr  r1, =_bss_end
    mov  r2, #0
.Lbss_loop:
    cmp  r0, r1
    bge  .Lcall_main
    str  r2, [r0], #4
    b    .Lbss_loop

    /* ── 6. Jump to main ── */
.Lcall_main:
    bl   main
.Lhang:
    b    .Lhang

/* ─────────────────────────────────────────────────────────────────────────
 * Default / Weak handlers
 * ───────────────────────────────────────────────────────────────────────── */
.thumb_func
.global Default_Handler
Default_Handler:
    b    Default_Handler

.weak HardFault_Handler
.thumb_func
.global HardFault_Handler
HardFault_Handler:
    b    Default_Handler

.weak SysTick_Handler
.thumb_func
.global SysTick_Handler
SysTick_Handler:
    b    Default_Handler

.weak PendSV_Handler
.thumb_func
.global PendSV_Handler
PendSV_Handler:
    b    Default_Handler

.weak USART3_IRQHandler
.thumb_func
.global USART3_IRQHandler
USART3_IRQHandler:
    b    Default_Handler

.weak TIM7_IRQHandler
.thumb_func
.global TIM7_IRQHandler
TIM7_IRQHandler:
    b    Default_Handler

.end