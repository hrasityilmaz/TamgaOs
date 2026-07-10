.syntax unified
.cpu    cortex-m7
.fpu    fpv5-d16
.thumb

.extern g_current_task
.extern g_next_task

.section .text, "ax", %progbits
.global PendSV_Handler
.thumb_func
.type   PendSV_Handler, %function

PendSV_Handler:
    mrs     r0, psp
    isb
    ldr     r3, =g_current_task
    ldr     r2, [r3]
    cbz     r2, PendSV_restore

    /*task using FPU? EXC_RETURN bit4=0 means yes */
    tst     lr, #0x10
    it      eq
    vstmdbeq r0!, {s16-s31}         /* push high FPU regs if used */
    stmdb   r0!, {r4-r11, lr}       /* push R4-R11 + EXC_RETURN */
    str     r0, [r2, #0]            /* save SP */

PendSV_restore:
    ldr     r3, =g_next_task
    ldr     r2, [r3]
    ldr     r3, =g_current_task
    str     r2, [r3]
    ldr     r0, [r2, #0]            /* load next task SP */
    ldmia   r0!, {r4-r11, lr}       /* pop R4-R11 + EXC_RETURN */
    tst     lr, #0x10
    it      eq
    vldmiaeq r0!, {s16-s31}         /* pop high FPU regs if used */
    msr     psp, r0
    isb
    bx      lr                      /* EXC_RETURN controls FPU/PSP/Thread */

.size PendSV_Handler, . - PendSV_Handler
.end
