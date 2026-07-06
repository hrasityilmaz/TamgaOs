.syntax unified
.cpu    cortex-m7
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
    stmdb   r0!, {r4-r11}
    str     r0, [r2, #0]

PendSV_restore:
    ldr     r3, =g_next_task
    ldr     r2, [r3]    
    ldr     r3, =g_current_task
    str     r2, [r3]
    ldr     r0, [r2, #0]
    ldmia   r0!, {r4-r11}
    msr     psp, r0
    isb
    movw    lr, #0xFFFD
    movt    lr, #0xFFFF
    bx      lr

.size PendSV_Handler, . - PendSV_Handler
.end
