
void _start(void)
{
  volatile unsigned short *VGA = (volatile unsigned short *) 0xB8000;
  const char *msg = "Kernel OK __C__";

  for(int i=0;i<msg[i]; i++){
    VGA[i] = (0x0F << 8) | msg[i];
  }

  for(;;){
    __asm__ volatile("hlt");
  }
}
