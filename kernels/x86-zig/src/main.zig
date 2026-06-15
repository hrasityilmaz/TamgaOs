const serial = @import("serial.zig");
const idt = @import("idt.zig");
const isr = @import("isr.zig");
const gdt_debug = @import("gdt_debug.zig");

const VGA = @as([*]volatile u16, @ptrFromInt(0xB8000));

// gdt written in assembly for a reason :)
extern fn gdt_load() void;

fn print(msg: []const u8, offset: usize) void {
    for (msg, 0..) |c, i| {
        VGA[offset + i] = (@as(u16, 0x0F) << 8) | c;
    }
}

export fn _start() noreturn {
    gdt_load();
    serial.init();

    gdt_debug.dumpLoadedGdtr();
    gdt_debug.dumpSegments();

    //serial.write("TamgaOs Booting...\n");
    comptime {
        _ = @import("isr.zig");
    }
    idt.init();
    //idt.dumpEntry(0);
    //

    //serial.write("TRIGGER INT 0\n");
    //asm volatile ("int $0");
    //serial.write("AFTER INT 0\n");

    //serial.write("TRIGGER #DE\n");
    //asm volatile (
    //    \\mov $1, %eax
    //    \\xor %edx, %edx
    //    \\xor %ecx, %ecx
    //    \\div %ecx
    //);

    //serial.write("AFTER REAL #DE\n");
    serial.write("IDT OK\n");
    idt.dumpEntry(0);

    //asm volatile("div %[z]" : :[z] "r" (@as(u32, 0)));

    print("TamgaOs", 0);
    print("GDT OK", 80);
    print("Kernel OK", 160);

    while (true) asm volatile ("hlt");
}
