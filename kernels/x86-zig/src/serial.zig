// https://wiki.osdev.org/Serial_Ports
//
// When on crashed and some error if vga will not work
// we can send messages to emulator logs
//

const COM = 0x3F8; // COM1

// inline assembly on zig AT&T syntax
fn outb(port: u16, value: u8) void {
    asm volatile ("outb %[val], %[port]"
        :
        : [val] "{al}" (value),
          [port] "N{dx}" (port),
    );
}

fn inb(port: u16) u8 {
    return asm volatile ("inb %[port], %[val]"
        : [val] "={al}" (-> u8),
        : [port] "N{dx}" (port),
    );
}

pub fn isTransmitEmpty() bool {
    return (inb(COM + 5) & 0x20) != 0;
}

pub fn writeByte(byte: u8) void {
    while (!isTransmitEmpty()) {}
    outb(COM, byte);
}

pub fn write(msg: []const u8) void {
    for (msg) |c| {
        writeByte(c);
    }
}

fn hxDigit(v: u4) u8 {
    if (v < 10) {
        return @as(u8, '0') + @as(u8, v); //'0' + v;
    }
    return @as(u8, 'A') + (@as(u8, v) - 10); //'A' + (v - 10);
}

pub fn writeHex32(value: u32) void {
    write("0x");
    var shift: i32 = 28; // 28 bit shift
    while (shift >= 0) : (shift -= 4) {
        const digit: u4 = @truncate(value >> @intCast(shift));
        writeByte(hxDigit(digit));
    }
}

pub fn writeHex16(value: u16) void {
    writeHex32(value);
}

pub fn writeHex8(value: u8) void {
    write("0x");
    var shift: i32 = 4;
    while (shift >= 0) : (shift -= 4) {
        const digit: u4 = @truncate(value >> @intCast(shift));
        writeByte(hxDigit(digit));
    }
}

pub fn init() void {
    outb(COM + 1, 0x00); // interrupt disable
    outb(COM + 3, 0x80); // DLAB on
    outb(COM + 0, 0x01); // baud 115200
    outb(COM + 1, 0x00);
    outb(COM + 3, 0x03); // 8 bit, no parity, 1 stop
    outb(COM + 2, 0xC7); // FIFO enable
    outb(COM + 4, 0x0B); // IRQ enable, RTS/DSR set
}
