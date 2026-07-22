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
 *   GPIO_Type / UART_Type / WDOG_Type / CAN_Type register layouts
 *   onto these fixed addresses.
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

/* ────────────────────────────────────────────────────────────
 * FlexCAN0 — struct layout and bit masks defined here directly
 * (not present in MK64F12REGS.h), based on the K64 Sub-Family
 * Reference Manual FlexCAN chapter register map.
 * ──────────────────────────────────────────────────────────── */

typedef struct {
  volatile uint32_t MCR;
  volatile uint32_t CTRL1;
  volatile uint32_t TIMER;
  uint32_t RESERVED_0;
  volatile uint32_t RXMGMASK;
  volatile uint32_t RX14MASK;
  volatile uint32_t RX15MASK;
  volatile uint32_t ECR;
  volatile uint32_t ESR1;
  uint32_t RESERVED_1;
  volatile uint32_t IMASK1;
  uint32_t RESERVED_2;
  volatile uint32_t IFLAG1;
  volatile uint32_t CTRL2;
  volatile const uint32_t ESR2;
  uint32_t RESERVED_3[2];
  volatile const uint32_t CRCR;
  volatile uint32_t RXFGMASK;
  volatile const uint32_t RXFIR;
  uint32_t RESERVED_4[12];
  struct {
    volatile uint32_t CS;
    volatile uint32_t ID;
    volatile uint32_t WORD0;
    volatile uint32_t WORD1;
  } MB[16];
} CAN_Type;

/* Chapter 4: Memory Map */
#define FLEXCAN0_BASE_ADDR (0x40024000UL)
/* MISRA 11.4 deviation applied below — see header comment */
#define FLEXCAN0 ((CAN_Type *)(FLEXCAN0_BASE_ADDR))

/* SIM->SCGC6 FLEXCAN0 clock gate bit (bit 4) */
#define SIM_SCGC6_FLEXCAN0_MASK (0x10UL)

/* MCR bit fields */
#define CAN_MCR_MDIS_MASK    (0x80000000UL)
#define CAN_MCR_FRZ_MASK     (0x40000000UL)
#define CAN_MCR_HALT_MASK    (0x10000000UL)
#define CAN_MCR_FRZACK_MASK  (0x01000000UL)

/* CTRL1 bit fields */
#define CAN_CTRL1_PROPSEG_SHIFT (0U)
#define CAN_CTRL1_PROPSEG(x)    (((uint32_t)(x) << CAN_CTRL1_PROPSEG_SHIFT) & 0x7UL)
#define CAN_CTRL1_LPB_MASK      (0x1000UL)
#define CAN_CTRL1_CLKSRC_MASK   (0x2000UL)
#define CAN_CTRL1_PSEG2_SHIFT   (16U)
#define CAN_CTRL1_PSEG2(x)      (((uint32_t)(x) << CAN_CTRL1_PSEG2_SHIFT) & 0x70000UL)
#define CAN_CTRL1_PSEG1_SHIFT   (19U)
#define CAN_CTRL1_PSEG1(x)      (((uint32_t)(x) << CAN_CTRL1_PSEG1_SHIFT) & 0x380000UL)
#define CAN_CTRL1_RJW_SHIFT     (22U)
#define CAN_CTRL1_RJW(x)        (((uint32_t)(x) << CAN_CTRL1_RJW_SHIFT) & 0xC00000UL)
#define CAN_CTRL1_PRESDIV_SHIFT (24U)
#define CAN_CTRL1_PRESDIV(x)    (((uint32_t)(x) << CAN_CTRL1_PRESDIV_SHIFT) & 0xFF000000UL)

/* Message Buffer CS word bit fields */
#define CAN_CS_CODE_SHIFT (24U)
#define CAN_CS_CODE_MASK  (0xF000000UL)
#define CAN_CS_CODE(x)    (((uint32_t)(x) << CAN_CS_CODE_SHIFT) & CAN_CS_CODE_MASK)
#define CAN_CS_SRR_MASK   (0x400000UL)
#define CAN_CS_IDE_MASK   (0x200000UL)
#define CAN_CS_RTR_MASK   (0x100000UL)
#define CAN_CS_DLC_SHIFT  (16U)
#define CAN_CS_DLC_MASK   (0xF0000UL)
#define CAN_CS_DLC(x)     (((uint32_t)(x) << CAN_CS_DLC_SHIFT) & CAN_CS_DLC_MASK)

/* Message Buffer ID word bit fields (standard 11-bit ID) */
#define CAN_ID_STD_SHIFT (18U)
#define CAN_ID_STD_MASK  (0x1FFC0000UL)
#define CAN_ID_STD(x)    (((uint32_t)(x) << CAN_ID_STD_SHIFT) & CAN_ID_STD_MASK)

#endif /* MMIO_DEVIATION_H */