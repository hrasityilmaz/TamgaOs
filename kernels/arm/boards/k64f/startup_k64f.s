.syntax unified
.cpu    cortex-m4
.thumb

.section .vectors, "a", %progbits
.align 2
_vectors:
    /* Core exceptions */
    .word  _stack_top           /* 0  Stack top            */
    .word  Reset_Handler        /* 1  Reset                */
    .word  Default_Handler      /* 2  NMI                  */
    .word  Default_Handler      /* 3  HardFault            */
    .word  Default_Handler      /* 4  MemManage            */
    .word  Default_Handler      /* 5  BusFault             */
    .word  Default_Handler      /* 6  UsageFault           */
    .word  0                    /* 7  Reserved             */
    .word  0                    /* 8  Reserved             */
    .word  0                    /* 9  Reserved             */
    .word  0                    /* 10 Reserved             */
    .word  Default_Handler      /* 11 SVC                  */
    .word  Default_Handler      /* 12 DebugMon             */
    .word  0                    /* 13 Reserved             */
    .word  Default_Handler      /* 14 PendSV               */
    .word  SysTick_Handler      /* 15 SysTick              */

    /* Peripheral IRQs (IRQ0 - IRQ47) */
    .word  Default_Handler      /* IRQ0  DMA0              */
    .word  Default_Handler      /* IRQ1  DMA1              */
    .word  Default_Handler      /* IRQ2  DMA2              */
    .word  Default_Handler      /* IRQ3  DMA3              */
    .word  Default_Handler      /* IRQ4  DMA4              */
    .word  Default_Handler      /* IRQ5  DMA5              */
    .word  Default_Handler      /* IRQ6  DMA6              */
    .word  Default_Handler      /* IRQ7  DMA7              */
    .word  Default_Handler      /* IRQ8  DMA8              */
    .word  Default_Handler      /* IRQ9  DMA9              */
    .word  Default_Handler      /* IRQ10 DMA10             */
    .word  Default_Handler      /* IRQ11 DMA11             */
    .word  Default_Handler      /* IRQ12 DMA12             */
    .word  Default_Handler      /* IRQ13 DMA13             */
    .word  Default_Handler      /* IRQ14 DMA14             */
    .word  Default_Handler      /* IRQ15 DMA15             */
    .word  Default_Handler      /* IRQ16 DMA Error         */
    .word  Default_Handler      /* IRQ17 MCM               */
    .word  Default_Handler      /* IRQ18 FTFE              */
    .word  Default_Handler      /* IRQ19 Read Collision    */
    .word  Default_Handler      /* IRQ20 LVD LVW           */
    .word  Default_Handler      /* IRQ21 LLWU              */
    .word  Default_Handler      /* IRQ22 WDOG/EWM          */
    .word  Default_Handler      /* IRQ23 RNG               */
    .word  Default_Handler      /* IRQ24 I2C0              */
    .word  Default_Handler      /* IRQ25 I2C1              */
    .word  Default_Handler      /* IRQ26 SPI0              */
    .word  Default_Handler      /* IRQ27 SPI1              */
    .word  Default_Handler      /* IRQ28 I2S0 Tx           */
    .word  Default_Handler      /* IRQ29 I2S0 Rx           */
    .word  Default_Handler      /* IRQ30 Reserved          */
    .word  Default_Handler      /* IRQ31 UART0 Status      */
    .word  Default_Handler      /* IRQ32 UART0 Error       */
    .word  Default_Handler      /* IRQ33 UART1 Status      */
    .word  Default_Handler      /* IRQ34 UART1 Error       */
    .word  Default_Handler      /* IRQ35 UART2 Status      */
    .word  Default_Handler      /* IRQ36 UART2 Error       */
    .word  Default_Handler      /* IRQ37 UART3 Status      */
    .word  Default_Handler      /* IRQ38 UART3 Error       */
    .word  Default_Handler      /* IRQ39 ADC0              */
    .word  Default_Handler      /* IRQ40 CMP0              */
    .word  Default_Handler      /* IRQ41 CMP1              */
    .word  Default_Handler      /* IRQ42 FTM0              */
    .word  Default_Handler      /* IRQ43 FTM1              */
    .word  Default_Handler      /* IRQ44 FTM2              */
    .word  Default_Handler      /* IRQ45 CMT               */
    .word  Default_Handler      /* IRQ46 RTC Alarm         */
    .word  Default_Handler      /* IRQ47 RTC Seconds       */

    /* IRQ48-51: PIT Channels */
    .word  PIT0_IRQHandler      /* IRQ48 PIT CH0  ← OS tick */
    .word  Default_Handler      /* IRQ49 PIT CH1           */
    .word  Default_Handler      /* IRQ50 PIT CH2           */
    .word  Default_Handler      /* IRQ51 PIT CH3           */

    /* IRQ52+ geri kalanlar - ihtiyaç oldukça eklenecek */
    .word  Default_Handler      /* IRQ52 PDB0              */
    .word  Default_Handler      /* IRQ53 USB OTG           */
    .word  Default_Handler      /* IRQ54 USB Charger       */
    .word  Default_Handler      /* IRQ55 Reserved          */
    .word  Default_Handler      /* IRQ56 DAC0              */
    .word  Default_Handler      /* IRQ57 MCG               */
    .word  Default_Handler      /* IRQ58 LPTMR0            */
    .word  Default_Handler      /* IRQ59 PORTA             */
    .word  Default_Handler      /* IRQ60 PORTB             */
    .word  Default_Handler      /* IRQ61 PORTC             */
    .word  Default_Handler      /* IRQ62 PORTD             */
    .word  Default_Handler      /* IRQ63 PORTE             */
    .word  Default_Handler      /* IRQ64 SWI               */
    .word  Default_Handler      /* IRQ65 SPI2              */
    .word  Default_Handler      /* IRQ66 UART4 Status      */
    .word  Default_Handler      /* IRQ67 UART4 Error       */
    .word  Default_Handler      /* IRQ68 Reserved          */
    .word  Default_Handler      /* IRQ69 Reserved          */
    .word  Default_Handler      /* IRQ70 CMP2              */
    .word  Default_Handler      /* IRQ71 FTM3              */
    .word  Default_Handler      /* IRQ72 DAC1              */
    .word  Default_Handler      /* IRQ73 ADC1              */
    .word  Default_Handler      /* IRQ74 I2C2              */
    .word  Default_Handler      /* IRQ75 CAN0 OE           */
    .word  Default_Handler      /* IRQ76 CAN0 Error        */
    .word  Default_Handler      /* IRQ77 CAN0 Tx Warn      */
    .word  Default_Handler      /* IRQ78 CAN0 Rx Warn      */
    .word  Default_Handler      /* IRQ79 CAN0 Wake         */
    .word  Default_Handler      /* IRQ80 ENET Timer        */
    .word  Default_Handler      /* IRQ81 ENET Tx           */
    .word  Default_Handler      /* IRQ82 ENET Rx           */
    .word  Default_Handler      /* IRQ83 ENET Error        */

/* --------------------------------------------------------
 * Flash Configuration Block (0x400)
 * FRDM-K64F: Bu bölüm 0x400 adresinde OLMALI
 * -------------------------------------------------------- */
.section .flashconfig, "a", %progbits
.align 2
    .word 0xFFFFFFFF        /* Backdoor key 0-3  */
    .word 0xFFFFFFFF        /* Backdoor key 4-7  */
    .word 0xFFFFFFFF        /* Flash protection  */
    .word 0xFFFFFFFE        /* Security: unsecured */

/* --------------------------------------------------------
 * Reset Handler
 * -------------------------------------------------------- */
.section .text, "ax", %progbits
.thumb_func
.global Reset_Handler
Reset_Handler:
    /* 1. Data segmentini flash'tan RAM'e kopyala */
    ldr  r0, =_data_load
    ldr  r1, =_data_start
    ldr  r2, =_data_end
    b    .data_copy_check
.data_copy_loop:
    ldr  r3, [r0], #4
    str  r3, [r1], #4
.data_copy_check:
    cmp  r1, r2
    blt  .data_copy_loop

    /* 2. BSS segmentini sıfırla */
    ldr  r0, =_bss_start
    ldr  r1, =_bss_end
    mov  r2, #0
    b    .bss_zero_check
.bss_zero_loop:
    str  r2, [r0], #4
.bss_zero_check:
    cmp  r0, r1
    blt  .bss_zero_loop
	
    bl   main
.hang:
    b    .hang

.thumb_func
.global Default_Handler
Default_Handler:
    b    Default_Handler

.weak PIT0_IRQHandler
.thumb_func
.global PIT0_IRQHandler
PIT0_IRQHandler:
    b    Default_Handler
	
.weak SysTick_Handler
.thumb_func
.global SysTick_Handler
SysTick_Handler:
    b    Default_Handler

.end