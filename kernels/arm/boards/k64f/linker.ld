ENTRY(Reset_Handler)

MEMORY
{
  FLASH        (rx)  : ORIGIN = 0x00000000, LENGTH = 1024K
  m_data       (rwx) : ORIGIN = 0x1FFF0000, LENGTH = 64K
  m_data_2     (rwx) : ORIGIN = 0x20000000, LENGTH = 192K
}

_stack_top = ORIGIN(m_data_2) + LENGTH(m_data_2);

SECTIONS
{
  .vectors :
  {
    KEEP(*(.vectors))
  } > FLASH

  .flashconfig 0x00000400 :
  {
    KEEP(*(.flashconfig))
  } > FLASH

  .text :
  {
    *(.text*)
    *(.rodata*)
    . = ALIGN(4);
    *(.kernel_asm*)
  } > FLASH

  .data :
  {
    . = ALIGN(4);
    _data_start = .;
    *(.data*)
    . = ALIGN(4);
    _data_end = .;
  } > m_data_2 AT > FLASH

  _data_load = LOADADDR(.data);

  .bss :
  {
    . = ALIGN(4);
    _bss_start = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _bss_end = .;
  } > m_data_2
  
  .noinit (NOLOAD) :
  {
    . = ALIGN(4);
    _noinit_start = .;
    *(.noinit*)
    . = ALIGN(4);
    _noinit_end = .;
  } > m_data_2
}
