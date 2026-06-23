/*
 * hardfault_handler.s
 * TamgaOS - HardFault: stack -> C handler
 */

.syntax unified
.cpu    cortex-m4
.thumb

.extern hardfault_c_handler

.global HardFault_Handler
.thumb_func
.type   HardFault_Handler, %function

HardFault_Handler:
    tst     lr, #4
    ite     eq
    mrseq   r0, msp
    mrsne   r0, psp
    b       hardfault_c_handler

.size HardFault_Handler, . - HardFault_Handler
