ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 1M
  RAM   (rwx) : ORIGIN = 0x1FFF0000, LENGTH = 256K
}

_stack_top = ORIGIN(RAM) + LENGTH(RAM) & ~7;

SECTIONS
{
  /* Vector table - 0x00000000 */
  .vectors :
  {
    KEEP(*(.vectors))
  } > FLASH

  /* Flash Configuration Field
  * Must be at 0x400, K64F RM Chapter 28
  */
  .flashconfig 0x00000400 :
  {
    KEEP(*(.flashconfig))
  } > FLASH

  /* Code and read-only data */
  .text :
  {
    *(.text*)
    *(.rodata*)
    . = ALIGN(4);
  } > FLASH

  /* Initialized data - copied from FLASH to RAM */
  .data :
  {
    . = ALIGN(4);
    _data_start = .;
    *(.data*)
    _data_end = .;
  } > RAM AT > FLASH

  _data_load = LOADADDR(.data); /* must be after daat 1!!!! */

  .bss :
  {
    . = ALIGN(4);
    _bss_start = .;
    *(.bss*)
    *(COMMON)
    _bss_end = .;
  } > RAM
}
