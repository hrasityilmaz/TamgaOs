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
C_SRCS  += kernel/core/queue.c
# For tests close main and open only one test all have main() !!
C_SRCS  += drivers/stm32h753zi/uart.c
C_SRCS  += drivers/stm32h753zi/rcc.c
C_SRCS += drivers/stm32h753zi/systick.c
C_SRCS += drivers/stm32h753zi/i2c.c
C_SRCS += drivers/stm32h753zi/pwm.c
C_SRCS += actuators/servo.c
# C_SRCS += src/stm32h753zi/main.c
# C_SRCS += tests/test_queue_priority_order.c
# C_SRCS += tests/test_mpu_stack_guard.c
# C_SRCS += tests/test_timeout.c
# C_SRCS += tests/test_event_flags.c
# C_SRCS += tests/stm/test_fdcan_real_bus.c
# C_SRCS += tests/stm/test_fdcan_loopback_final.c
# C_SRCS += tests/stm/pwm_test.c
# C_SRCS += tests/stm/toggle_test.c
C_SRCS += tests/stm/servo_sweep_test.c
C_SRCS += sensors/mpu6050.c
C_SRCS += drivers/stm32h753zi/fdcan.c
C_SRCS += controls/kalman.c
C_SRCS += kernel/core/hardfault.c
C_SRCS += kernel/core/fault_log.c
C_SRCS += kernel/core/event.c
C_SRCS += drivers/stm32h753zi/iwdg.c


PYOCD_TARGET = stm32h743xx
BOARD_INCLUDES += -Iinclude/stm32h753zi
BOARD_INCLUDES += -Iinclude/sensors
BOARD_INCLUDES += -Iinclude/controls
BOARD_INCLUDES += -Iinclude/actuators