struct gdt_entry {
  unsigned short limit_low;
  unsigned short base_low;
  unsigned char base_mid;
  unsigned char access;
  unsigned char granularity;
  unsigned char base_high;
} __attribute__((packed));

struct gdt_ptr {
  unsigned short limit;
  unsigned int base;
} __attribute__((packed));

static struct gdt_entry gdt[3];
static struct gdt_ptr gdtr;

static void make_entry(int i, unsigned int base, unsigned int limit,
                       unsigned char access, unsigned char gran) {
  gdt[i].base_low = (base & 0xFFFF);
  gdt[i].base_mid = (base >> 16) & 0xFF;
  gdt[i].base_high = (base >> 24) & 0xFF;

  gdt[i].limit_low = (limit & 0xFFFF);
  gdt[i].granularity = (limit >> 16) & 0x0F;
  gdt[i].granularity |= (gran & 0xF0);
  gdt[i].access = access;
}

void gdt_init(void) {
  gdtr.limit = (sizeof(struct gdt_entry) * 3) - 1;
  gdtr.base = (unsigned int)&gdt;

  make_entry(0, 0, 0x00000, 0x00, 0x00); /* null       */
  make_entry(1, 0, 0xFFFFF, 0x9A, 0xCF); /* code  0x08 */
  make_entry(2, 0, 0xFFFFF, 0x92, 0xCF); /* data  0x10 */

  __asm__ volatile("cli");
  __asm__ volatile("lgdt (%0)" : : "r"(&gdtr));
  __asm__ volatile("ljmp $0x08, $1f\n"
                   "1:             \n"
                   "mov $0x10, %%ax     \n"
                   "mov %%ax, %%ds      \n"
                   "mov %%ax, %%es      \n"
                   "mov %%ax, %%fs      \n"
                   "mov %%ax, %%gs      \n"
                   "mov %%ax, %%ss      \n"
                   :
                   :
                   : "eax", "memory");
}
