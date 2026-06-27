.syntax unified
.cpu    cortex-m4
.thumb

.section .kernel_asm, "ax", %progbits
.global sched_start_asm
.thumb_func
.type   sched_start_asm, %function

sched_start_asm:
    cpsid   i
    ldr     r1, [r0, #0] 
    ldmia   r1!, {r4-r11}
    msr     psp, r1
    isb
    mov     r0, #2
    msr     control, r0
    isb  /* if pending interrupt */
    cpsie   i
    isb
    
    /* r1: R0(0),R1(4),R2(8),R3(12),R12(16),LR(20),PC(24),xPSR(28) */
    /* mrs r1, psp  */       
    /* ldr r2, [r1, #24] */    
    ldr     r2, [r1, #24]
    bx      r2 /* not bx lr !!!! */

.size sched_start_asm, . - sched_start_asm
.end
