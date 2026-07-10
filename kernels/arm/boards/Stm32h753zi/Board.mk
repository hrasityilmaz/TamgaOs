CPU_FLAGS  = -mcpu=cortex-m7
CPU_FLAGS += -mthumb
CPU_FLAGS += -mfpu=fpv5-d16
CPU_FLAGS += -mfloat-abi=hard
# CPU_FLAGS += -mfloat-abi=soft

STARTUP      = boards/stm32h753zi/startup_stm32h753zi.s
LDSCRIPT     = boards/stm32h753zi/linker.ld
BOARD_DEFINE = BOARD_STM32H753ZI

CPU_FLAGS += -DCORE_CLOCK_HZ=480000000UL

AS_SRCS += kernel/arch/cortex_m7/sched_start.s
AS_SRCS += kernel/arch/cortex_m7/pendsv_handler.s

C_SRCS  += kernel/core/scheduler.c
C_SRCS  += kernel/core/mutex.c
C_SRCS  += kernel/core/semaphore.c
C_SRCS  += drivers/stm32h753zi/uart.c

C_SRCS  += drivers/stm32h753zi/rcc.c
C_SRCS += drivers/stm32h753zi/systick.c
C_SRCS += drivers/stm32h753zi/i2c.c
C_SRCS += src/stm32h753zi/main.c
C_SRCS += sensors/mpu6050.c
C_SRCS += kernel/core/hardfault.c


PYOCD_TARGET = stm32h743xx
# CFLAGS += -Ikernels/arm/drivers/stm32h753zi
BOARD_INCLUDES += -Iinclude/stm32h753zi
BOARD_INCLUDES += -Iinclude/sensors