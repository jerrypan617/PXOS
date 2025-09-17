# PXOS Kernel Makefile
# Professional kernel development structure

# Compiler and tools
CC = i686-elf-gcc
AS = nasm
LD = i686-elf-ld
OBJCOPY = i686-elf-objcopy

# Directories
BUILD_DIR = build
ARCH_DIR = arch/x86
DRIVERS_DIR = drivers
FS_DIR = fs
KERNEL_DIR = kernel
LIB_DIR = lib

# Compiler flags
CFLAGS = -ffreestanding -c -g -O2 -Wall -Wextra -I$(KERNEL_DIR) -I$(DRIVERS_DIR)/vga -I$(DRIVERS_DIR)/keyboard -I$(FS_DIR) -I$(LIB_DIR)
ASFLAGS = -f bin
ASMFLAGS = -f elf32

# Source files
KERNEL_SRC = $(KERNEL_DIR)/kernel.c \
             $(KERNEL_DIR)/interrupt.c \
             $(KERNEL_DIR)/memory.c \
             $(KERNEL_DIR)/process/process.c \
             $(KERNEL_DIR)/syscall.c

DRIVERS_SRC = $(DRIVERS_DIR)/vga/vga.c \
              $(DRIVERS_DIR)/keyboard/keyboard.c

FS_SRC = $(FS_DIR)/filesystem.c

LIB_SRC = $(LIB_DIR)/string.c

# Assembly files
BOOT_ASM = $(ARCH_DIR)/boot.asm
INTERRUPT_ASM = $(ARCH_DIR)/interrupt_asm.asm
PAGING_ASM = $(ARCH_DIR)/paging_asm_simple.asm
SYSCALL_ASM = $(ARCH_DIR)/syscall_asm.asm

# Object files
KERNEL_OBJ = $(BUILD_DIR)/kernel.o \
             $(BUILD_DIR)/interrupt.o \
             $(BUILD_DIR)/memory.o \
             $(BUILD_DIR)/process.o \
             $(BUILD_DIR)/syscall.o

DRIVERS_OBJ = $(BUILD_DIR)/vga.o \
              $(BUILD_DIR)/keyboard.o

FS_OBJ = $(BUILD_DIR)/filesystem.o

LIB_OBJ = $(BUILD_DIR)/string.o

ASM_OBJ = $(BUILD_DIR)/interrupt_asm.o \
          $(BUILD_DIR)/paging_asm.o \
          $(BUILD_DIR)/syscall_asm.o

# Build targets
BOOT_BIN = $(BUILD_DIR)/boot.bin
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
FLOPPY_IMG = floppy.img

# Default target
all: $(FLOPPY_IMG)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Bootloader
$(BOOT_BIN): $(BOOT_ASM) | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Kernel object files
$(BUILD_DIR)/kernel.o: $(KERNEL_DIR)/kernel.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: $(KERNEL_DIR)/interrupt.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/memory.o: $(KERNEL_DIR)/memory.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/process.o: $(KERNEL_DIR)/process/process.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/syscall.o: $(KERNEL_DIR)/syscall.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Driver object files
$(BUILD_DIR)/vga.o: $(DRIVERS_DIR)/vga/vga.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/keyboard.o: $(DRIVERS_DIR)/keyboard/keyboard.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Filesystem object files
$(BUILD_DIR)/filesystem.o: $(FS_DIR)/filesystem.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Library object files
$(BUILD_DIR)/string.o: $(LIB_DIR)/string.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Assembly object files
$(BUILD_DIR)/interrupt_asm.o: $(INTERRUPT_ASM) | $(BUILD_DIR)
	$(AS) $(ASMFLAGS) $< -o $@

$(BUILD_DIR)/paging_asm.o: $(PAGING_ASM) | $(BUILD_DIR)
	$(AS) $(ASMFLAGS) $< -o $@

$(BUILD_DIR)/syscall_asm.o: $(SYSCALL_ASM) | $(BUILD_DIR)
	$(AS) $(ASMFLAGS) $< -o $@

# Link kernel
$(KERNEL_BIN): $(KERNEL_OBJ) $(DRIVERS_OBJ) $(FS_OBJ) $(LIB_OBJ) $(ASM_OBJ) linker.ld
	$(LD) -T linker.ld --oformat binary $(KERNEL_OBJ) $(DRIVERS_OBJ) $(FS_OBJ) $(LIB_OBJ) $(ASM_OBJ) -o $@

# Create floppy image
$(FLOPPY_IMG): $(BOOT_BIN) $(KERNEL_BIN)
	@echo "Creating blank floppy image..."
	dd if=/dev/zero of=$@ bs=512 count=2880
	@echo "Writing bootloader to first sector..."
	dd if=$(BOOT_BIN) of=$@ bs=512 count=1 conv=notrunc
	@echo "Writing kernel to subsequent sectors..."
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=1 conv=notrunc
	@echo "Floppy image created successfully!"

# Run in QEMU
run: $(FLOPPY_IMG)
	@echo "Starting QEMU..."
	qemu-system-i386 -fda $(FLOPPY_IMG) -monitor stdio -boot a

# Clean build files
clean:
	@echo "Cleaning build files..."
	rm -rf $(BUILD_DIR)
	rm -f $(FLOPPY_IMG)
	rm -f os_image.bin

# Debug build (with debug symbols)
debug: CFLAGS += -DDEBUG -g3
debug: all

# Release build (optimized)
release: CFLAGS += -O3 -DNDEBUG
release: all

# Help target
help:
	@echo "Available targets:"
	@echo "  all      - Build the kernel and create floppy image"
	@echo "  run      - Build and run in QEMU"
	@echo "  clean    - Remove all build files"
	@echo "  debug    - Build with debug symbols"
	@echo "  release  - Build optimized release version"
	@echo "  help     - Show this help message"

# Phony targets
.PHONY: all run clean debug release help