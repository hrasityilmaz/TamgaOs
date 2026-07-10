#ifndef TAMGAOS_COMPILER_H
#define TAMGAOS_COMPILER_H

/*
 * tamgaos_compiler.h
 *
 * MISRA-C:2012 Rule 1.2 / Directive 1.1 — Justified Deviation
 *
 * Rationale: This project requires GNU compiler extensions
 * (__attribute__) for hardware-level operations that standard C
 * cannot express (memory alignment for MPU regions, naked functions
 * for hand-written context-switch entry points).
 *
 * These extensions are isolated in this single header so that:
 *   1) The deviation is documented and auditable in one place
 *   2) Porting to a different compiler only requires editing this file
 *   3) No raw __attribute__ usage appears elsewhere in the codebase
 */

#if defined(__GNUC__)
#define TAMGAOS_ALIGNED(x) __attribute__((aligned(x)))
#define TAMGAOS_NAKED      __attribute__((naked))
#else
#error "TamgaOS: unsupported compiler - TAMGAOS_ALIGNED/TAMGAOS_NAKED must be defined"
#endif

#endif /* TAMGAOS_COMPILER_H */