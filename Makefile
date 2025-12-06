# Makefile for ARM kernel with sources in src/

# Toolchain
CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

# Directories
BUILD_DIR ?= build

# Files - automatically find all .c files in src/ and subdirectories
BOOT = src/boot.s
SRC = $(shell find src -name '*.c')
LINKER = src/linker.ld
OUTPUT = kernel

# Generate object file names from source files
OBJS = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRC))

# Flags
CFLAGS = -mcpu=arm926ej-s -marm -O2 -nostdlib -ffreestanding
LDFLAGS = -T $(LINKER)

# Targets
all: $(OUTPUT).bin

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Pattern rule to compile .c files to .o files
$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/boot.o: $(BOOT) | $(BUILD_DIR)
	$(AS) -o $@ $<

$(OUTPUT).elf: $(BUILD_DIR)/boot.o $(OBJS) $(LINKER)
	$(LD) $(LDFLAGS) -o $(BUILD_DIR)/$(OUTPUT).elf $(BUILD_DIR)/boot.o $(OBJS)

$(OUTPUT).bin: $(OUTPUT).elf
	$(OBJCOPY) -O binary $(BUILD_DIR)/$(OUTPUT).elf $(BUILD_DIR)/$(OUTPUT).bin

clean:
	rm -rf $(BUILD_DIR)
