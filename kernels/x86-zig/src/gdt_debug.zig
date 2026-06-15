const serial = @import("serial.zig");

const GdtPtr = packed struct {
    limit: u16,
    base: u32,
};

pub fn dumpLoadedGdtr() void {
    var gdtr: GdtPtr = .{ .limit = 0, .base = 0 };

    asm volatile ("sgdt %[ptr]"
        : [ptr] "=m" (gdtr),
        :
        : .{ .memory = true });

    serial.write("GDTR base=");
    serial.writeHex32(gdtr.base);
    serial.write(" limit=");
    serial.writeHex16(gdtr.limit);
    serial.write("\n");
}

fn readCs() u16 {
    return asm volatile (
        \\push %cs
        \\pop %[out]
        : [out] "=r" (-> u16),
    );
}

fn readDs() u16 {
    return asm volatile (
        \\mov %ds, %[out]
        : [out] "=r" (-> u16),
    );
}

fn readSs() u16 {
    return asm volatile (
        \\mov %ss, %[out]
        : [out] "=r" (-> u16),
    );
}

pub fn dumpSegments() void {
    serial.write("CS=");
    serial.writeHex16(readCs());

    serial.write(" DS=");
    serial.writeHex16(readDs());

    serial.write(" SS=");
    serial.writeHex16(readSs());
    serial.write("\n");
}