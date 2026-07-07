.syntax unified
.cpu    cortex-m7
.fpu    fpv5-d16
.thumb

.section .kernel_asm, "ax", %progbits
.global sched_start_asm
.thumb_func
.type   sched_start_asm, %function

sched_start_asm:
    cpsid   i
    ldr     r1, [r0, #0]            /* r1 = task->sp */
    ldmia   r1!, {r4-r11, lr}       /* pop R4-R11 + EXC_RETURN */
    tst     lr, #0x10
    it      eq
    vldmiaeq r1!, {s16-s31}

    msr     psp, r1
    isb
    mov     r0, #2
    msr     control, r0
    isb
    cpsie   i
    isb
    ldr     r2, [r1, #24]           /* PC from exception frame */
    bx      r2                      /* jump to task — not bx lr! */

.size sched_start_asm, . - sched_start_asm
.end