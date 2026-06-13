.syntax unified
        .cpu    cortex-m4
        .thumb

// --------------------------------------------------------
// Vector Table
// Sequence important! See ARM docs:
// https://developer.arm.com/documentation/107565/0101/Use-case-examples/Generic-Information/What-is-inside-a-program-image-/Vector-table
// --------------------------------------------------------
        .section .vectors, "a", %progbits
        .align 2
_vectors:
        .word  _stack_top       // Stack top
        .word  Reset_Handler    // Reset
        .word  Default_Handler  // NMI
        .word  Default_Handler  // HardFault
        .word  Default_Handler  // MemManage
        .word  Default_Handler  // BusFault
        .word  Default_Handler  // UsageFault
        .word  0
        .word  0
        .word  0
        .word  0
        .word  Default_Handler  // SVC
        .word  Default_Handler  // DebugMon
        .word  0
        .word  Default_Handler  // PendSV
        .word  SysTick_Handler  // SysTick

// --------------------------------------------------------
// Flash Configuration Block (0x400)
// Must be at 0x400 - chip reads this on every boot!
// Last word 0xFFFFFFFE = unsecured, otherwise chip locks itself
// --------------------------------------------------------
        .section .FlashConfig, "a", %progbits
        .align 2
        .word 0xFFFFFFFF   // Backdoor key 0-3
        .word 0xFFFFFFFF   // Backdoor key 4-7
        .word 0xFFFFFFFF   // Flash protection
        .word 0xFFFFFFFE   // Security: unsecured (bit0=0)

// --------------------------------------------------------
// Reset Handler
// --------------------------------------------------------
        .section .text, "ax", %progbits
        .thumb_func
        .global Reset_Handler
Reset_Handler:

        // Disable Watchdog
        // Unlock sequence: write 0xC520 then 0xD928 to WDOG_UNLOCK (0x4005200E)
        ldr  r0, =0x4005200E
        ldr  r1, =0xC520
        strh r1, [r0]
        ldr  r1, =0xD928
        strh r1, [r0]
        // Disable watchdog: write to WDOG_STCTRLH (0x40052000)
        ldr  r0, =0x40052000
        ldr  r1, =0x01D2
        strh r1, [r0]

        // Copy .data from flash to RAM
        ldr  r0, =_data_load
        ldr  r1, =_data_start
        ldr  r2, =_data_end
        b    .data_copy_check
.data_copy_loop:
        ldr  r3, [r0], #4
        str  r3, [r1], #4
.data_copy_check:
        cmp  r1, r2
        blt  .data_copy_loop

        // Zero .bss
        ldr  r0, =_bss_start
        ldr  r1, =_bss_end
        mov  r2, #0
        b    .bss_zero_check
.bss_zero_loop:
        str  r2, [r0], #4
.bss_zero_check:
        cmp  r0, r1
        blt  .bss_zero_loop

        bl   main

.hang:
        b    .hang

// --------------------------------------------------------
// Default Handler
// --------------------------------------------------------
        .thumb_func
        .global Default_Handler
Default_Handler:
        b    Default_Handler

        .end
