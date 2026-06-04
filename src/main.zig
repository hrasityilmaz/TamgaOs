const serial = @import("serial.zig");
const VGA = @as([*]volatile u16, @ptrFromInt(0xB8000));

// gdt written in assembly
extern fn gdt_load() void;

fn print(msg: []const u8, offset: usize) void {
    for (msg, 0..) |c, i| {
        VGA[offset + i] = (@as(u16, 0x0F) << 8) | c;
    }
}

export fn _start() noreturn {
    gdt_load();
    serial.init();
    serial.write("TamgaOs Booting...\n");

    print("TamgaOs", 0);
    print("GDT OK", 80);

    while (true) asm volatile ("hlt");
}
