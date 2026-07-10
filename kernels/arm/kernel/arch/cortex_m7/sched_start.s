.syntax unified
.cpu    cortex-m7
.fpu    fpv5-d16
.thumb

.section .text, "ax", %progbits

/*
 * sched_start_asm — triggers SVC 0 to start first task
 * SVC_Handler does the actual context restore
 */
.global sched_start_asm
.thumb_func
.type   sched_start_asm, %function

sched_start_asm:
    cpsid   i
    cpsie   i
    cpsie   f
    dsb
    isb
    svc     0
    nop

.size sched_start_asm, . - sched_start_asm

/* 
 * SVC_Handler — restores first task context via exception return
*/
.global SVC_Handler
.thumb_func
.type   SVC_Handler, %function

SVC_Handler:
    ldr     r3, =g_current_task
    ldr     r1, [r3]            /* r1 = g_current_task */
    ldr     r0, [r1]            /* r0 = task->sp */

    ldmia   r0!, {r4-r11, r14} /* restore R4-R11 + EXC_RETURN */

    tst     r14, #0x10
    it      eq
    vldmiaeq r0!, {s16-s31}

    msr     psp, r0
    isb
    mov     r0, #0
    msr     basepri, r0
    bx      r14

.size SVC_Handler, . - SVC_Handler
.end