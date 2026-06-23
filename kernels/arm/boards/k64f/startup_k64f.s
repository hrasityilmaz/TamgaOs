.syntax unified
.cpu    cortex-m4
.thumb

.section .vectors, "a", %progbits
.align 2
_vectors:
    .word  _stack_top
    .word  Reset_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  0
    .word  0
    .word  0
    .word  0
    .word  Default_Handler
    .word  Default_Handler
    .word  0
    .word  PendSV_Handler
    .word  SysTick_Handler
    .word  Default_Handler 
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler 
    .word  Default_Handler
    .word  Default_Handler  
    .word  Default_Handler  
    .word  Default_Handler  
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler 
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  UART0_RX_TX_IRQHandler  /* IRQ31 UART0 */
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  PIT0_IRQHandler /*IRQ48 PIT0*/
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler 
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler

.section .flashconfig, "a", %progbits
.align 2
    .word 0xFFFFFFFF
    .word 0xFFFFFFFF
    .word 0xFFFFFFFF
    .word 0xFFFFFFFE

.section .text, "ax", %progbits
.thumb_func
.global Reset_Handler
Reset_Handler:
    /* 1. Watchdog'u kapat - BSS dongusunden once */
    /* WDOG_UNLOCK: 0x40052000 + 0x0E = 0x4005200E */
    ldr  r0, =0x4005200EU
    ldr  r1, =0xC520
    strh r1, [r0]
    ldr  r1, =0xD928
    strh r1, [r0]
    /* WDOG_STCTRLH: 0x40052000, bit0=0 disable */
    ldr  r0, =0x40052000U
    ldr  r1, =0x0010
    strh r1, [r0]

    /* 2. BSS sifirla */
    ldr  r0, =_bss_start
    ldr  r1, =_bss_end
    subs r2, r1, r0
    beq  .Lcall_main
    mov  r1, #0
.Lbss_loop:
    str  r1, [r0], #4
    subs r2, r2, #4
    bgt  .Lbss_loop

.Lcall_main:
    bl   main
.Lhang:
    b    .Lhang

.thumb_func
.global Default_Handler
Default_Handler:
    b    Default_Handler

.weak UART0_RX_TX_IRQHandler
.thumb_func
.global UART0_RX_TX_IRQHandler
UART0_RX_TX_IRQHandler:
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

.weak PendSV_Handler
.thumb_func
.global PendSV_Handler
PendSV_Handler:
    b    Default_Handler

.end
