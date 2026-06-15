const serial = @import("serial.zig");

const IDT_ENTRIES = 256;

// Here extern important C_ABI issue...
const IDTEntry = extern struct {
    offset_low: u16,
    selector: u16,
    zero: u8,
    type_attr: u8,
    offset_high: u16,
}; // @sizeof => 8bytes...

const IdtPtr = packed struct {
    limit: u16,
    base: u32,
};

var idt: [IDT_ENTRIES]IDTEntry = undefined;
var idt_ptr: IdtPtr = undefined;

fn SetEntry(n: u8, handler: u32) void {
    idt[n].offset_low = @truncate(handler);
    idt[n].selector = 0x08;
    idt[n].zero = 0;
    idt[n].type_attr = 0x8E;
    idt[n].offset_high = @truncate(handler >> 16);
}

extern fn isr0() void;
extern fn isr6() void;
extern fn isr8() void;
extern fn isr13() void;
extern fn isr14() void;

pub fn init() void {
    idt_ptr.limit = @sizeOf(@TypeOf(idt)) - 1;
    idt_ptr.base = @intFromPtr(&idt);

    for (0..IDT_ENTRIES) |i| {
        SetEntry(@intCast(i), 0);
    }

    SetEntry(0, @intFromPtr(&isr0));
    SetEntry(6, @intFromPtr(&isr6));
    SetEntry(8, @intFromPtr(&isr8));
    SetEntry(13, @intFromPtr(&isr13));
    SetEntry(14, @intFromPtr(&isr14));

    asm volatile ("lidt (%[ptr])"
        :
        : [ptr] "r" (&idt_ptr),
    );
}

pub fn dumpEntry(index: usize) void {
    const e = idt[index];

    serial.write("low=");
    serial.writeHex16(e.offset_low);

    serial.write(" high=");
    serial.writeHex16(e.offset_high);

    serial.write(" full=");
    serial.writeHex32((@as(u32, e.offset_high) << 16) | @as(u32, e.offset_low));

    serial.write(" type=");
    serial.writeHex8(e.type_attr);
    serial.write("\n");
}
