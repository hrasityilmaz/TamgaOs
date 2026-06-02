; 0x00   NULL
; 0x08   Kernel code
; 0x10   Kernel data

bits 32

section .data
align 8

gdt_start:
gdt_null:
        dq 0x0000000000000000

;Loading as .little_endian
gdt_kernel_code:
        dw 0xFFFF      ;limit[15:0]
        dw 0x0000      ;base[15:0]
        db 0x00        ;base[23:16]
        db 10011010b   ;P=1 DPL=00 Type=1010 (code, readable, non-conforming)
        db 11011111b   ;G = 1 S = 1 Type = 1010
        db 0x00        ;base[32:24]

gdt_kernel_data:
    dw 0xFFFF           ; limit[15:0]
    dw 0x0000           ; base[15:0]
    db 0x00             ; base[23:16]
    db 10010010b        ; P=1 DPL=00 S=1 Type=0010 (data, writable, expand-up)
    db 11001111b        ; G=1 D/B=1 (32-bit) AVL=0 L=0 | limit[19:16]=1111
    db 0x00             ; base[31:24]

gdt_end:

gdt_ptr:
        dw gdt_end - gdt_start - 1
        dd gdt_start

section .text
global gdt_load

gdt_load:
      cli ; close interrupts  
      lgdt [gdt_ptr]
jmp 0x08:.flush

.flush:
        mov ax, 0x10
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax

        ret
