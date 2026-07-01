ENTRY(Reset_Handler)

/*
 * STM32H753ZI Memory Map
 * REF: dm00314099 Section 2.3 — MUST CHECK AGAIN !!!
 *
 * FLASH_BANK1 : 0x08000000  1MB
 * FLASH_BANK2 : 0x08100000  1MB
 * ITCM        : 0x00000000  64KB  — zero wait-state, CPU only
 * DTCM        : 0x20000000  128KB — zero wait-state, CPU only (no DMA!)
 * AXI         : 0x24000000  512KB — D1 domain
 * SRAM1       : 0x30000000  128KB — D2 domain
 * SRAM2       : 0x30020000  128KB — D2 domain
 * SRAM3       : 0x30040000  32KB  — D2 domain
 * SRAM4       : 0x38000000  64KB  — D3 domain (BDMA only)
 * BKPSRAM     : 0x38800000  4KB   — backup SRAM (survives reset if VBAT present)
 */

MEMORY
{
  FLASH_BANK1 (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
  FLASH_BANK2 (rx)  : ORIGIN = 0x08100000, LENGTH = 1024K
  ITCM        (rwx) : ORIGIN = 0x00000000, LENGTH = 64K
  DTCM        (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
  AXI         (rwx) : ORIGIN = 0x24000000, LENGTH = 512K
  SRAM1       (rwx) : ORIGIN = 0x30000000, LENGTH = 128K
  SRAM2       (rwx) : ORIGIN = 0x30020000, LENGTH = 128K
  SRAM3       (rwx) : ORIGIN = 0x30040000, LENGTH = 32K
  SRAM4       (rwx) : ORIGIN = 0x38000000, LENGTH = 64K
  BKPSRAM     (rwx) : ORIGIN = 0x38800000, LENGTH = 4K
}

/* starts with DTCM */
_stack_top = ORIGIN(DTCM) + LENGTH(DTCM);

SECTIONS
{
  /* -- Vector table — must be at FLASH_BANK1 start (0x08000000) -- */
  .vectors :
  {
    KEEP(*(.vectors))
  } > FLASH_BANK1

  /* -- Code + read-only data -- */
  .text :
  {
    *(.text*)
    *(.rodata*)
    . = ALIGN(4); /* .word  */
    *(.kernel_asm*)           /* PendSV, SysTick, sched_start etc */
  } > FLASH_BANK1

  /* -- Initialised data: stored in FLASH_BANK1, copied to DTCM by startup -- */
  /* startup problems must control */
  .data :
  {
    . = ALIGN(4);
    _data_start = .;
    *(.data*)
    . = ALIGN(4);
    _data_end = .;
  } > DTCM AT > FLASH_BANK1

  _data_load = LOADADDR(.data);

  /* .bss */
  .bss :
  {
    . = ALIGN(4);
    _bss_start = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _bss_end = .;
  } > DTCM

  /* 
  No-init section (crash logs, debug state, survives reset) 
  will use BKPSRAM for logs.  
  */
  /*
  .noinit (NOLOAD) :
  {
    . = ALIGN(4);
    _noinit_start = .;
    *(.noinit*)
    . = ALIGN(4);
    _noinit_end = .;
  } > DTCM
  */

  /* --AXI SRAM -- */
  .axi_bss (NOLOAD) :
  {
    . = ALIGN(4);
    _axi_bss_start = .;
    *(.axi_bss*)
    . = ALIGN(4);
    _axi_bss_end = .;
  } > AXI

  /*-- D2 domain --*/
  .sram1_bss (NOLOAD) :
  {
    . = ALIGN(4);
    *(.sram1_bss*)
    . = ALIGN(4);
  } > SRAM1

  .bkpsram (NOLOAD) :
  {
    . = ALIGN(4);
    *(.bkpsram*)
    . = ALIGN(4);
  } > BKPSRAM
}
