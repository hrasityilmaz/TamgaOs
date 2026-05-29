const VGA = @as([*]volatile u16, @ptrFromInt(0xB8000));

export fn _start() noreturn {
    const msg = "KERNEL OK";
    for (msg, 0..) |c, i| {
        VGA[i] = (@as(u16, 0x0F) << 8) | c;
    }
    while (true) asm volatile ("hlt");
}
