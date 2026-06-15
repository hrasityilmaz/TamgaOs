#include "isr.h"
#include "serial.h"

void isr_handler(register_t *r) {
  serial_write("EXCEPTION:");
  switch (r->int_no) {
  case 0:
    serial_write("#DE Divide Error\n");
    break;
  case 6:
    serial_write("#UD Invalid Opcode\n");
    break;
  case 8:
    serial_write("#DF Double Fault\n");
    break;
  case 13:
    serial_write("#GP General Protection\n");
    break;
  case 14:
    serial_write("#PF Page Fault\n");
    break;
  default:
    serial_write("Unknown Exception\n");
  }
  for (;;) {
    __asm__ volatile("hlt");
  }
}
