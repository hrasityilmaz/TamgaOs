#include "gdt.h"

static volatile unsigned short *const VGA = (volatile unsigned short *)0xB8000;

static void print(const char *msg, unsigned int offset) {
  unsigned int i = 0;

  while (msg[i] != '\0') {
    VGA[offset + i] = (0x0F << 8) | msg[i];
    i++;
  }
}

void _start(void) {
  gdt_init();
  volatile unsigned short *VGA = (volatile unsigned short *)0xB8000;

  print("TamgaOS __C__", 0);
  print("GDT OK __C__", 80);
  print("Kernel OK __C__", 160);
  for (;;) {
    __asm__ volatile("hlt");
  }
}
