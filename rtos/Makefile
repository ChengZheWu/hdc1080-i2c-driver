# RTOS Makefile for MPS2-AN385 + FreeRTOS

CROSS   = arm-none-eabi-
CC      = $(CROSS)gcc
LD      = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
SIZE    = $(CROSS)size

TARGET   = hdc1080_rtos
LDSCRIPT = ../bare-metal/scripts/mps2-an385.ld

CPU_FLAGS = -mcpu=cortex-m3 -mthumb

FREERTOS_SRC  = freertos/kernel
FREERTOS_PORT = freertos/portable/ARM_CM3

CFLAGS = $(CPU_FLAGS) -O0 -g3 -Wall -Wextra -ffreestanding -fno-builtin -nostartfiles -Iinclude -I$(FREERTOS_SRC)/include -I$(FREERTOS_PORT)

LDFLAGS = $(CPU_FLAGS) -T$(LDSCRIPT) -nostartfiles --specs=nano.specs -Wl,--gc-sections -Wl,-Map=$(TARGET).map

APP_SRCS = src/startup.c src/uart.c src/i2c.c src/hdc1080.c src/main.c

RTOS_SRCS = $(FREERTOS_SRC)/tasks.c $(FREERTOS_SRC)/queue.c $(FREERTOS_SRC)/list.c $(FREERTOS_SRC)/timers.c $(FREERTOS_SRC)/event_groups.c $(FREERTOS_SRC)/stream_buffer.c $(FREERTOS_PORT)/port.c freertos/portable/heap_4.c

SRCS = $(APP_SRCS) $(RTOS_SRCS)
OBJS = $(SRCS:.c=.o)

.PHONY: all clean run

all: $(TARGET).elf $(TARGET).bin
	$(SIZE) $(TARGET).elf

$(TARGET).elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET).elf
	@echo "=== Starting QEMU with FreeRTOS ==="
	@echo "=== Press Ctrl+A then X to exit ==="
	qemu-system-arm -machine mps2-an385 -cpu cortex-m3 -kernel $(TARGET).elf -nographic -semihosting

clean:
	find . -name "*.o" -delete
	rm -f $(TARGET).elf $(TARGET).bin $(TARGET).map
