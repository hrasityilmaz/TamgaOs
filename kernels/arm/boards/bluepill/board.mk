
# boards/bluepill/board.mk
# STM32F103C8T6 (Cortex-M3, no FPU)

CPU_FLAGS  = -mcpu=cortex-m3
CPU_FLAGS += -mthumb

STARTUP    = boards/bluepill/startup_bluepill.s
LDSCRIPT   = boards/bluepill/linker.ld
BOARD_DEFINE = BOARD_BLUEPILL
