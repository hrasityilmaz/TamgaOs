#define COM 0x3F8

static void outb(unsigned short port, unsigned char value) {
  __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static unsigned char inb(unsigned short port) {
  unsigned char value;
  __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
  return value;
}

static int is_transmit_empty(void) { return inb(COM + 5) & 0x20; }

void serial_init(void) {
  outb(COM + 1, 0x00); // interrupt disable
  outb(COM + 3, 0x80); // DLAB on
  outb(COM + 0, 0x01); // baud 115200
  outb(COM + 1, 0x00);
  outb(COM + 3, 0x03); // 8 bit, no parity, 1 stop
  outb(COM + 2, 0xC7); // FIFO enable
  outb(COM + 4, 0x0B); // IRQ enable, RTS/DSR set
}

void serial_write(const char *msg) {
  for (int i = 0; msg[i] != '\0'; i++) {
    while (!is_transmit_empty()) {
    }
    outb(COM, msg[i]);
  }
}
