TARGET = tamgaos

BOARD ?= k64f

CC      = arm-none-eabi-gcc
AS      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

include boards/$(BOARD)/board.mk

CFLAGS  = $(CPU_FLAGS)
CFLAGS += -O0 -g #-O0 -DNDEBUG #-O2 -DNDEBUG  # -O0 -g
CFLAGS += -Wall -Wextra
# CFLAGS += -ffunction-sections
# CFLAGS += -fdata-sections
CFLAGS += -std=gnu11
CFLAGS += -Iinclude
CFLAGS += -Iboards/$(BOARD)
CFLAGS += -D$(BOARD_DEFINE)
CFLAGS += $(BOARD_INCLUDES)

ASFLAGS  = $(CPU_FLAGS)
ASFLAGS += -g

LDFLAGS  = -T $(LDSCRIPT)
LDFLAGS += -Wl,-Map=$(TARGET)_$(BOARD).map
LDFLAGS += -nostartfiles
LDFLAGS += -Wl,-u,sched_start_asm
LDFLAGS += -Wl,-u,SVC_Handler


# C_SRCS  += src/main.c
AS_SRCS += $(STARTUP)

C_OBJS   = $(C_SRCS:.c=.o)
AS_OBJS  = $(AS_SRCS:.s=.o)
OBJS     = $(AS_OBJS) $(C_OBJS)

all: $(TARGET)_$(BOARD).bin $(TARGET)_$(BOARD).hex
	$(SIZE) $(TARGET)_$(BOARD).elf

$(TARGET)_$(BOARD).elf: $(OBJS)
	$(CC) $(CPU_FLAGS) $(LDFLAGS) $(OBJS) -o $@

$(TARGET)_$(BOARD).bin: $(TARGET)_$(BOARD).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET)_$(BOARD).hex: $(TARGET)_$(BOARD).elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) -c $< -o $@

flash: $(TARGET)_$(BOARD).bin
	pyocd flash -t $(PYOCD_TARGET) $(TARGET)_$(BOARD).bin

clean:
	powershell -Command "'$(OBJS) $(TARGET)_$(BOARD).elf $(TARGET)_$(BOARD).bin $(TARGET)_$(BOARD).hex $(TARGET)_$(BOARD).map' -split ' ' | ForEach-Object { Remove-Item -ErrorAction SilentlyContinue -Force $$_ }"

.PHONY: all flash clean
