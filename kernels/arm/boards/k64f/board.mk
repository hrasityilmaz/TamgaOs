CPU_FLAGS  = -mcpu=cortex-m4
CPU_FLAGS += -mthumb
CPU_FLAGS += -mfloat-abi=soft

STARTUP      = boards/k64f/startup_k64f.s
LDSCRIPT     = boards/k64f/linker.ld
BOARD_DEFINE = BOARD_K64F

CPU_FLAGS += -DCORE_CLOCK_HZ=120000000UL

AS_SRCS += kernel/arch/cortex_m4/sched_start.s
AS_SRCS += kernel/arch/cortex_m4/pendsv_handler.s

C_SRCS  += kernel/core/scheduler.c
C_SRCS  += drivers/k64f/pit.c drivers/k64f/uart.c drivers/k64f/mcg.c kernel/core/mutex.c kernel/core/semaphore.c
C_SRCS += kernel/core/hardfault.c
C_SRCS += src/k64f/main.c

PYOCD_TARGET = k64f

CFLAGS += -Ikernels/arm/drivers/k64f
BOARD_INCLUDES += -Iinclude/k64f
