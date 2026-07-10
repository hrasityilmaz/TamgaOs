.syntax unified
.cpu    cortex-m4
.thumb

.section .vectors, "a", %progbits
.align 2
_vectors:
    .word  _stack_top
    .word  Reset_Handler
    .word  Default_Handler
    .word  HardFault_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  Default_Handler
    .word  0
    .word  0
    .word  0
    .word  0
    .word  SVC_Handler
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
    /* 1. WDOG unlock + disable */
    ldr  r0, =0x4005200EU
    ldr  r1, =0xC520
    strh r1, [r0]
    ldr  r1, =0xD928
    strh r1, [r0]
    ldr  r0, =0x40052000U
    ldr  r1, =0x0010
    strh r1, [r0]

    /* .data: Flash -> RAM */
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

    /* 3. BSS zeroed */
.Lbss_init:
    ldr  r0, =_bss_start
    ldr  r1, =_bss_end
    mov  r2, #0
.Lbss_loop:
    cmp  r0, r1
    bge  .Lcall_main
    str  r2, [r0], #4
    b    .Lbss_loop

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

.weak HardFault_Handler
.thumb_func
.global HardFault_Handler
HardFault_Handler:
    b    Default_Handler

.weak SVC_Handler
.thumb_func
.global SVC_Handler
SVC_Handler:
    b    Default_Handler

.end