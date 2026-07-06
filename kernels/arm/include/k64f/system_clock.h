#ifndef SYSTEM_CLOCK_H
#define SYSTEM_CLOCK_H

/*
 * system_clock.h
 * TamgaOS - System Clock
 * PLL: 50MHz / 15 * 36 = 120MHz
 */

#ifndef CORE_CLOCK_HZ
#define CORE_CLOCK_HZ    (120000000UL)  /* PLL (OUTDIV1 = /1) */
#endif

#define EXTAL0_HZ        (50000000UL)   /* External ethernet clock */
#define BUS_CLOCK_HZ     (60000000UL)   /* Core / 2            */
#define FLEXBUS_CLOCK_HZ (40000000UL)   /* Core / 3            */
#define FLASH_CLOCK_HZ   (24000000UL)   /* Core / 5            */
#define UART0_CLOCK_HZ   CORE_CLOCK_HZ  /* UART0 core clk      */
#define PIT_CLOCK_HZ     BUS_CLOCK_HZ   /* PIT bus clk         */

#endif /* SYSTEM_CLOCK_H */