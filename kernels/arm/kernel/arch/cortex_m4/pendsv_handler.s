.syntax unified
.cpu    cortex-m4
.fpu    fpv4-sp-d16
.thumb

.extern g_current_task
.extern g_next_task

.section .kernel_asm, "ax", %progbits
.global PendSV_Handler
.thumb_func
.type   PendSV_Handler, %function

PendSV_Handler:
    mrs     r0, psp
    isb
    ldr     r3, =g_current_task
    ldr     r2, [r3]
    cbz     r2, PendSV_restore
    tst     lr, #0x10
    it      eq
    vstmdbeq r0!, {s16-s31} 
    stmdb   r0!, {r4-r11, lr}
    str     r0, [r2, #0]

PendSV_restore:
    ldr     r3, =g_next_task
    ldr     r2, [r3]
    ldr     r3, =g_current_task
    str     r2, [r3]

    ldr     r0, [r2, #0]         
    ldmia   r0!, {r4-r11, lr}   
    tst     lr, #0x10
    it      eq
    vldmiaeq r0!, {s16-s31}     

    msr     psp, r0
    isb
    bx      lr

.size PendSV_Handler, . - PendSV_Handler
.end