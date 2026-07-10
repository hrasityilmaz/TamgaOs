#ifndef TAMGAOS_MMIO_H
#define TAMGAOS_MMIO_H

#include <stdint.h>

/*
 * tamgaos_mmio.h
 *
 * MISRA-C:2012 Rule 11.4 / Rule 11.6 — Justified Deviation
 *
 * Rationale: Accessing memory-mapped peripheral/core registers on
 * ARMv7-M requires converting a fixed integer address to an object
 * pointer, which is inherently implementation-defined. Standard C
 * offers no alternative for this (it is the fundamental mechanism by
 * which any embedded MMIO driver operates).
 *
 * This conversion is isolated to the two macros below so that:
 *   1) The deviation is documented and auditable in one place
 *   2) All register definitions across the codebase use a single,
 *      reviewed pattern instead of ad hoc casts scattered per file
 */
#define TAMGAOS_REG32(addr) (*(volatile uint32_t *)(addr))
#define TAMGAOS_REG8(addr)  (*(volatile uint8_t  *)(addr))

#endif /* TAMGAOS_MMIO_H */