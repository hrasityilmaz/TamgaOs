const GdtEntry = packed struct {
    limit_low: u16,
    base_low: u16,
    base_mid: u8,
    access: u8,
    granularity: u8,
    base_high: u8,
}; // @Sizeof 8bytes

const GdtPtr = packed struct {
    limit: u16,
    base: u32,
}; // @Sizeof 6 bytes

var gdt: [3]GdtEntry align(8) = undefined;
var gdt_ptr: GdtPtr align(4) = undefined;

fn makeEntry(base: u32, limit: u32, access: u8, gran: u8) GdtEntry {
    return GdtEntry{
        .limit_low = @truncate(limit & 0xFFFF),
        .base_low = @truncate(base & 0xFFFF),
        .base_mid = @truncate((base >> 16) & 0xFF),
        .access = access,
        .granularity = @truncate(((limit >> 16) & 0x0F) | (gran & 0xF0)),
        .base_high = @truncate((base >> 24) & 0xFF),
    };
}

pub fn init() void {
    gdt[0] = makeEntry(0, 0, 0, 0);
    gdt[1] = makeEntry(0, 0xFFFFF, 0x9A, 0xCF);
    gdt[2] = makeEntry(0, 0xFFFFF, 0x92, 0xCF);

    gdt_ptr = GdtPtr{
        .limit = @sizeOf(@TypeOf(gdt)) - 1,
        .base = @intFromPtr(&gdt),
    };
    lgdt(&gdt_ptr);
}

fn lgdt(ptr: *const GdtPtr) void {
    asm volatile (
        \\ lgdt (%[p])
        \\ ljmp $0x08, $1f
        \\ 1:
        \\ mov $0x10, %%ax
        \\ mov %%ax, %%ds
        \\ mov %%ax, %%es
        \\ mov %%ax, %%fs
        \\ mov %%ax, %%gs
        \\ mov %%ax, %%ss
        :
        : [p] "r" (ptr),
        : .{ .ax = true, .memory = true });
}
