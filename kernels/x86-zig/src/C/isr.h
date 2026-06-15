#ifndef ISR_H
#define ISR_H

typedef struct {
  unsigned int gs, fs, es, ds;
  unsigned int edi, esi, ebp, esp;
  unsigned int ebx, edx, ecx, eax;
  unsigned int int_no, err_code;
  unsigned int eip, cs, eflags;
} register_t;

void isr_handler(register_t *r);

#endif
