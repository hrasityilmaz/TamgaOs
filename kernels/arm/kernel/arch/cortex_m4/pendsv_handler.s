.syntax unified
.cpu    cortex-m4
.thumb

.extern g_current_task
.extern g_next_task

.section .kernel_asm, "ax", %progbits
.global PendSV_Handler
.thumb_func
.type   PendSV_Handler, %function

/*
 * PendSV_Handler — context switch.
 * Cortex-M4 (FPU'suz), STM32'nin M7 versiyonuyla ayni stack formatini
 * kullanir (kernel/core/scheduler.c task_stack_init — board-bagimsiz):
 *   ... R4-R11, EXC_RETURN, R0-R3,R12,LR,PC,xPSR (hardware frame) ...
 * Tek fark: burada FPU yok, bu yuzden s16-s31 push/pop satirlari yok.
 */
PendSV_Handler:
    mrs     r0, psp
    isb

    ldr     r3, =g_current_task
    ldr     r2, [r3]
    cbz     r2, PendSV_restore

    stmdb   r0!, {r4-r11, lr}   /* push R4-R11 + EXC_RETURN (lr, PendSV entry'de zaten dogru deger) */
    str     r0, [r2, #0]        /* save SP */

PendSV_restore:
    ldr     r3, =g_next_task
    ldr     r2, [r3]
    ldr     r3, =g_current_task
    str     r2, [r3]

    ldr     r0, [r2, #0]        /* load next task SP */
    ldmia   r0!, {r4-r11, lr}   /* pop R4-R11 + EXC_RETURN */

    msr     psp, r0
    isb
    bx      lr                  /* EXC_RETURN — hardware R0-R3,R12,LR,PC,xPSR'i geri yukler */

.size PendSV_Handler, . - PendSV_Handler
.end