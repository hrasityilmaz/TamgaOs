# boards/k64f/board.mk
CPU_FLAGS  = -mcpu=cortex-m4
CPU_FLAGS += -mthumb
CPU_FLAGS += -mfpu=fpv4-sp-d16
CPU_FLAGS += -mfloat-abi=hard

STARTUP      = boards/k64f/startup_k64f.s
LDSCRIPT     = boards/k64f/linker.ld
BOARD_DEFINE = BOARD_K64F

CPU_FLAGS += -DCORE_CLOCK_HZ=120000000UL
#CPU_FLAGS += -DUSE_PLL