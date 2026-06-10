ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 1M
  RAM   (rwx) : ORIGIN = 0x1FFF0000, LENGTH = 256K
}

_stack_top = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  .vectors :
  {
    KEEP(*(.vectors))
  } > FLASH

  .FlashConfig 0x400 :
  {
    . = ALIGN(4);
    KEEP(*(.FlashConfig))
  } > FLASH

  .text :
  {
    *(.text*)
    *(.rodata*)
    . = ALIGN(4);
  } > FLASH

  _data_load = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _data_start = .;
    *(.data*)
    _data_end = .;
  } > RAM AT > FLASH

  .bss :
  {
    . = ALIGN(4);
    _bss_start = .;
    *(.bss*)
    *(COMMON)
    _bss_end = .;
  } > RAM
}