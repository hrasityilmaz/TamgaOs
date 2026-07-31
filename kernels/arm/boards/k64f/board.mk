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
C_SRCS  += kernel/core/mutex.c 
C_SRCS  += kernel/core/semaphore.c
C_SRCS  += kernel/core/queue.c 
C_SRCS  += kernel/core/event.c
C_SRCS  += kernel/core/hardfault_minimal.c
C_SRCS += kernel/core/timer.c
C_SRCS += kernel/core/deadline_monitor.c
C_SRCS += kernel/core/notify.c
C_SRCS += kernel/core/stack_monitor.c

# Drivers 
C_SRCS  += drivers/k64f/uart.c
C_SRCS  += drivers/k64f/mcg.c
C_SRCS  += drivers/k64f/flexcan.c
C_SRCS  += drivers/k64f/pwm.c
C_SRCS  += drivers/k64f/adc.c
C_SRCS  += actuators/k64f/servo.c


# TESTS #
# C_SRCS  += src/k64f/main.c
# C_SRCS  += tests/k64f/fpu_test.c
# C_SRCS  += tests/k64f/test_queue_priority_order.c
# C_SRCS  += tests/k64f/test_event_flags.c
# C_SRCS  += tests/k64f/test_flexcan_loopback.c
# C_SRCS  += tests/k64f/test_flexcan_real_bus.c
# C_SRCS  += tests/k64f/pwm_test.c
# C_SRCS  += tests/k64f/servo_sweep.c
# C_SRCS += tests/k64f/test_software_timer.c
# C_SRCS += tests/k64f/test_tickless_idle.c
# C_SRCS += tests/k64f/test_deadline_monitor.c
# C_SRCS += tests/k64f/test_isr_safe_primitives.c
# C_SRCS += tests/k64f/adc_pot_test.c
# C_SRCS += tests/k64f/esc_pot_test.c
# C_SRCS += tests/k64f/esc_pot_full_manual.c
# C_SRCS += tests/k64f/esc_motor_test.c
# C_SRCS += tests/k64f/pwm_4ch_test.c
C_SRCS += tests/k64f/pwm_4ch_pot_test.c
# C_SRCS += tests/k64f/test_task_notify.c
# C_SRCS += tests/k64f/test_stack_monitor.c

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
BOARD_INCLUDES += -Iinclude/actuators/k64f