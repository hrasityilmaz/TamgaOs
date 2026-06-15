#include "idt.h"

#define IDT_ENTRIES 256

struct idt_entry {
  unsigned short offset_low;
  unsigned short selector;
  unsigned char zero;
  unsigned char type_attr;
  unsigned short offset_high;
} __attribute__((packed));

struct idt_ptr {
  unsigned short limit;
  unsigned int base;
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idt_ptr;

extern void isr0(void);
extern void isr6(void);
extern void isr8(void);
extern void isr13(void);
extern void isr14(void);

static void setEntry(int n, unsigned int handler) {
  idt[n].offset_low = handler & 0xFFFF;
  idt[n].selector = 0x08;
  idt[n].zero = 0;
  idt[n].type_attr = 0x8E;
  idt[n].offset_high = (handler >> 16) & 0xFFFF;
}

void idt_init(void) {
  idt_ptr.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
  idt_ptr.base = (unsigned int)&idt;

  for (int i = 0; i < IDT_ENTRIES; i++) {
    setEntry(i, 0);
  }

  setEntry(0, (unsigned int)isr0);
  setEntry(6, (unsigned int)isr6);
  setEntry(8, (unsigned int)isr8);
  setEntry(13, (unsigned int)isr13);
  setEntry(14, (unsigned int)isr14);

  __asm__ volatile("lidt (%0)" : : "r"(&idt_ptr));
}
