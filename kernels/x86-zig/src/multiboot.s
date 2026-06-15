.section .multiboot, "a"
.align 8

.global multiboot_header
multiboot_header:

.long 0xE85250D6
.long 0
.long 24
.long 0x17ADAF12

.word 0
.word 0
.long 8

header_end:
