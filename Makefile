# Makefile for ARM kernel with sources in src/

# Toolchain
CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

# Directories
BUILD ?= build

# Files - automatically find all .c files in src/ and subdirectories
BOOT = src/boot.s
SRC = $(shell find src -name '*.c')
LINKER = src/linker.ld
OUTPUT = spark

# Generate object file names from source files
OBJS = $(patsubst src/%.c,$(BUILD)/%.o,$(SRC))

# Flags
CFLAGS = -mcpu=arm926ej-s -marm -O2 -nostdlib -ffreestanding -Isrc
LDFLAGS = -T $(LINKER)

# Targets
all: $(BUILD)/$(OUTPUT).bin

$(BUILD):
	mkdir -p $(BUILD)

# Pattern rule to compile .c files to .o files
$(BUILD)/%.o: src/%.c | $(BUILD)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/boot.o: $(BOOT) | $(BUILD)
	$(AS) -o $@ $<

%.elf: $(BUILD)/boot.o $(OBJS) $(LINKER)
	$(LD) $(LDFLAGS) -o $@ $(BUILD)/boot.o $(OBJS)

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -rf $(BUILD)
