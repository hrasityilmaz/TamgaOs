TARGET = tamgaos

CC      = arm-none-eabi-gcc
AS      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

# CPU flags
CPU_FLAGS  = -mcpu=cortex-m4
CPU_FLAGS += -mfpu=fpv4-sp-d16
CPU_FLAGS += -mfloat-abi=hard
CPU_FLAGS += -mthumb

# C flags
CFLAGS  = $(CPU_FLAGS)
CFLAGS += -O0 -g
CFLAGS += -Wall -Wextra
CFLAGS += -ffunction-sections
CFLAGS += -fdata-sections
CFLAGS += -std=gnu11
CFLAGS += -Iinclude

# Assembler flags
ASFLAGS  = $(CPU_FLAGS)
ASFLAGS += -g

# Linker flags
LDFLAGS  = -T linker/MK64F_minimal.ld
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(TARGET).map
LDFLAGS += -nostartfiles
#LDFLAGS += -specs=nosys.specs
#LDFLAGS += -specs=nano.specs

# Source files
#ORJ
# C_SRCS   = src/main.c startup/startup_MK64F12.c

#MINE
C_SRCS   = src/main.c src/uart.c
AS_SRCS  = startup/startup_k64f.s

# Object files
C_OBJS   = $(C_SRCS:.c=.o)
AS_OBJS  = $(AS_SRCS:.S=.o)
OBJS     = $(AS_OBJS) $(C_OBJS)

# -------------------------------------------------- #

all: $(TARGET).bin $(TARGET).hex
	$(SIZE) $(TARGET).elf

$(TARGET).elf: $(OBJS)
	$(CC) $(CPU_FLAGS) $(LDFLAGS) $(OBJS) -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

# C derleme
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Assembly derleme
%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@

# Flash'a yaz (pyocd kullanılıyor)
flash: $(TARGET).bin
	pyocd flash -t k64f $(TARGET).bin

clean:
	powershell -Command "'$(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).hex $(TARGET).map' -split ' ' | ForEach-Object { Remove-Item -ErrorAction SilentlyContinue -Force $$_ }"

.PHONY: all flash clean
