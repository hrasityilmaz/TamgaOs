.section .multiboot, "a"
.align 8

.global multiboot_header
multiboot_header:

.long 0xE85250D6          /* magic */
.long 0                    /* architecture: i386 */
.long 24                   /* header length (hardcoded) */
.long 0x17ADAF12           

/* End tag */
.word 0
.word 0
.long 8

header_end:
