const gdt = @import("gdt.zig");

const VGA = @as([*]volatile u16, @ptrFromInt(0xB8000));

fn print(msg: []const u8, offset: usize) void {
    for (msg, 0..) |c, i| {
        VGA[offset + i] = (@as(u16, 0x0F) << 8) | c;
    }
}

export fn _start() noreturn {
    gdt.init();

    print("TamgaOs", 0);
    print("GDT OK", 80);

    while (true) asm volatile ("hlt");
}
