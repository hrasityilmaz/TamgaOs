global isr0, isr6, isr8, isr13, isr14 ; #DE #UD  #DF #GP #PF all these errors cathing for now...
extern isr_handler ;on c and zig we will handle wilth this function..

isr0:
        push 0
        push 0
        jmp isr_common_stub

isr6:
        push 0
        push 6
        jmp isr_common_stub

isr8:
        push 8
        jmp isr_common_stub

isr13:
        push 13
        jmp isr_common_stub

isr14:
        push 14
        jmp isr_common_stub

isr_common_stub:
        pusha ; EAX ECX EDX EBX ESP EBP ESI EDI save all to stack
        push ds
        push es
        push fs
        push gs

        ; now only protected mode
        ; but for guarantee
        mov ax, 0x10
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax

        push esp ; C or Zig send address to function
        call isr_handler
        add esp, 4

        pop gs
        pop fs
        pop es
        pop ds
        popa
        add esp, 8
        iret
