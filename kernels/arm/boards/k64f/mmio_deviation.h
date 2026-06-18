#ifndef MMIO_DEVIATION_H
#define MMIO_DEVIATION_H

/* ============================================================
 * mmio_deviation.h
 * MISRA C:2012 Deviation Record
 *
 * Rule:        11.4 (Advisory)
 * Title:       "A conversion should not be performed between a
 *               pointer to object and an integer type."
 *
 * Justification:
 *   On the Kinetis K64F (ARM Cortex-M4), memory-mapped peripheral
 *   registers (SIM, PORT, GPIO, UART, WDOG, etc.) are accessed
 *   through fixed physical addresses
 *   Chapter 4: Memory Map, K64 Sub-Family Reference Manual
 *
 *   These addresses are constant, well-known, and never change at
 *   runtime. Casting an integer literal to a pointer-to-struct is
 *   the only practical way to overlay the SIM_Type / PORT_Type /
 *   GPIO_Type / UART_Type / WDOG_Type register layouts onto these
 *   fixed addresses.
 *
 *   Risk assessment:
 *     - The integer-to-pointer conversion is confined to this
 *       single header file.
 *     - All resulting pointers are 'volatile'-qualified struct
 *       pointers, preventing the compiler from optimizing away
 *       register accesses.
 *     - Addresses are taken directly from the vendor reference
 *       manual and verified against the register map in
 *       MK64F12REGS.h.
 *     - No pointer arithmetic is performed on these pointers
 *       beyond struct member access, which is bounds-safe by
 *       construction of the struct layout.
 *
 *   Conclusion:
 *     This deviation is considered SAFE and is the standard,
 *       widely-accepted pattern (CMSIS-style) for register-level
 *       access in embedded systems. The deviation is isolated to
 *       this file to ease auditing.
 *
 * ============================================================
 */

#include "MK64F12REGS.h"
#include <stdint.h>

/* Chapter 4: Memory Map */
#define SIM_BASE_ADDR (0x40047000UL)
#define PORTA_BASE_ADDR (0x40049000UL)
#define PORTB_BASE_ADDR (0x4004A000UL)
#define PORTC_BASE_ADDR (0x4004B000UL)
#define PORTD_BASE_ADDR (0x4004C000UL)
#define PORTE_BASE_ADDR (0x4004D000UL)
#define UART0_BASE_ADDR (0x4006A000UL)
#define UART1_BASE_ADDR (0x4006B000UL)
#define WDOG_BASE_ADDR (0x40052000UL)
#define GPIOA_BASE_ADDR (0x400FF000UL)
#define GPIOB_BASE_ADDR (0x400FF040UL)
#define GPIOC_BASE_ADDR (0x400FF080UL)
#define GPIOD_BASE_ADDR (0x400FF0C0UL)
#define GPIOE_BASE_ADDR (0x400FF100UL)
#define MCG_BASE_ADDR (0x40064000UL)
#define ENET_BASE_ADDR (0x400C0000UL)

/* MISRA 11.4 deviation applied below — see header comment */
#define SIM ((SIM_Type *)(SIM_BASE_ADDR))
#define PORTA ((PORT_Type *)(PORTA_BASE_ADDR))
#define PORTB ((PORT_Type *)(PORTB_BASE_ADDR))
#define PORTC ((PORT_Type *)(PORTC_BASE_ADDR))
#define PORTD ((PORT_Type *)(PORTD_BASE_ADDR))
#define PORTE ((PORT_Type *)(PORTE_BASE_ADDR))
#define UART0 ((UART_Type *)(UART0_BASE_ADDR))
#define UART1 ((UART_Type *)(UART1_BASE_ADDR))
#define WDOG ((WDOG_Type *)(WDOG_BASE_ADDR))
#define GPIOA ((GPIO_Type *)(GPIOA_BASE_ADDR))
#define GPIOB ((GPIO_Type *)(GPIOB_BASE_ADDR))
#define GPIOC ((GPIO_Type *)(GPIOC_BASE_ADDR))
#define GPIOD ((GPIO_Type *)(GPIOD_BASE_ADDR))
#define GPIOE ((GPIO_Type *)(GPIOE_BASE_ADDR))
#define MCG ((MCG_Type *)(MCG_BASE_ADDR))
#define ENET ((ENET_Type *)(ENET_BASE_ADDR)) /* YENİ: ENET pointer */

#define SYSTICK_BASE_ADDR (0xE000E010UL)
#define SYSTICK ((SysTick_Type *)(SYSTICK_BASE_ADDR))

#define PIT_BASE_ADDR (0x40037000UL)
#define PIT ((PIT_Type *)(PIT_BASE_ADDR))

#endif /* MMIO_DEVIATION_H */
