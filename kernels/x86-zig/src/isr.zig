const serial = @import("serial.zig");

pub const Registers = extern struct {
    gs: u32,
    fs: u32,
    es: u32,
    ds: u32,
    edi: u32,
    esi: u32,
    ebp: u32,
    esp: u32,
    ebx: u32,
    edx: u32,
    ecx: u32,
    eax: u32,
    int_no: u32,
    err_code: u32,
    eip: u32,
    cs: u32,
    eflags: u32,
};

pub export fn isr_handler(r: *Registers) callconv(.c) void {
    serial.write("EXCEPTION: ");

    serial.write("ISR CAUGHT\n");
    serial.write("int_no=");
    serial.writeHex32(r.int_no);
    serial.write("\n");
    switch (r.int_no) {
        0 => serial.write("#DE Divide Error\n"),
        6 => serial.write("#UD Invalid Opcode\n"),
        8 => serial.write("#GP General Protection\n"),
        13 => serial.write("#GP General Protection\n"),
        14 => serial.write("#PF Page Fault\n"),
        else => serial.write("Unknown\n"),
    }
    // for test comment ....
    while (true) asm volatile ("hlt");
}
