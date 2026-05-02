# Bare-metal Makefile for MPS2-AN385

CROSS   = arm-none-eabi-
CC      = $(CROSS)gcc
LD      = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
SIZE    = $(CROSS)size

TARGET   = hdc1080_baremetal
LDSCRIPT = scripts/mps2-an385.ld

CPU_FLAGS = -mcpu=cortex-m3 -mthumb

CFLAGS = $(CPU_FLAGS) -O0 -g3 -Wall -Wextra -ffreestanding -fno-builtin -nostdlib -Iinclude

LDFLAGS = $(CPU_FLAGS) -T$(LDSCRIPT) -nostdlib -Wl,--gc-sections -Wl,-Map=$(TARGET).map

SRCS = src/startup.c src/uart.c src/i2c.c src/hdc1080.c src/main.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean run debug

all: $(TARGET).elf $(TARGET).bin
	$(SIZE) $(TARGET).elf

$(TARGET).elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET).elf
	@echo "=== Starting QEMU (mps2-an385) ==="
	@echo "=== Press Ctrl+A then X to exit ==="
	qemu-system-arm -machine mps2-an385 -cpu cortex-m3 -kernel $(TARGET).elf -nographic -semihosting

debug: $(TARGET).elf
	qemu-system-arm -machine mps2-an385 -cpu cortex-m3 -kernel $(TARGET).elf -nographic -semihosting -S -gdb tcp::3333 &
	@echo "QEMU waiting for GDB on port 3333"

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).map
