CPU_FLAGS  = -mcpu=cortex-m4
CPU_FLAGS += -mthumb
# CPU_FLAGS += -mfloat-abi=soft
CPU_FLAGS += -mfpu=fpv4-sp-d16
CPU_FLAGS += -mfloat-abi=hard

STARTUP      = boards/k64f/startup_k64f.s
LDSCRIPT     = boards/k64f/linker.ld
BOARD_DEFINE = BOARD_K64F

CPU_FLAGS += -DCORE_CLOCK_HZ=120000000UL

AS_SRCS += kernel/arch/cortex_m4/sched_start.s
AS_SRCS += kernel/arch/cortex_m4/pendsv_handler.s

C_SRCS  += kernel/core/scheduler.c
C_SRCS  += drivers/k64f/uart.c
C_SRCS  += drivers/k64f/mcg.c
C_SRCS  += drivers/k64f/flexcan.c
C_SRCS  += kernel/core/mutex.c 
C_SRCS  += kernel/core/semaphore.c
C_SRCS  += kernel/core/queue.c 
C_SRCS  += kernel/core/event.c
C_SRCS  += kernel/core/hardfault_minimal.c
# C_SRCS  += src/k64f/main.c
# C_SRCS  += tests/k64f/fpu_test.c
# C_SRCS  += tests/k64f/test_queue_priority_order.c
# C_SRCS  += tests/k64f/test_event_flags.c
# C_SRCS  += tests/k64f/test_flexcan_loopback.c
C_SRCS  += tests/k64f/test_flexcan_real_bus.c

# Tick source selection — default: native ARM SysTick 
#   Usage: make BOARD=k64f TICK_SOURCE=pit
TICK_SOURCE ?= systick

ifeq ($(TICK_SOURCE),pit)
  C_SRCS += drivers/k64f/pit.c drivers/k64f/pit_systick_shim.c
  CPU_FLAGS += -DTICK_SOURCE_PIT=1
else
  C_SRCS += drivers/k64f/systick.c
endif

PYOCD_TARGET = k64f

BOARD_INCLUDES += -Iinclude/k64f