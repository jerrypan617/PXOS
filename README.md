# PXOS Kernel

A 32-bit x86 operating system kernel written in C and assembly, featuring a modular architecture with process management, memory management, filesystem support, and an interactive shell.

## Architecture

- **Target Architecture**: x86-32 (32-bit protected mode)
- **Bootloader**: Custom assembly bootloader that loads the kernel from floppy disk
- **Memory Model**: Protected mode with paging support
- **Display**: VGA text mode (80x25)

## Core Components

### 1. Boot System
- Assembly-based bootloader (`boot.asm`)
- Loads kernel from floppy disk sectors
- Switches from **real mode** to **32-bit protected mode**
- Sets up basic Global Descriptor Table (GDT)

### 2. Memory Management
- 4KB pages with page directory and page tables
- Support for virtual-to-physical address translation
- Kernel heap allocation with `kmalloc`/`kfree`
- Separate kernel and user space memory areas
- Tracking of allocated/free memory

### 3. Process Management
- Process Control Block
- 5 Process States of New, Ready, Running, Blocked, Terminated
- 4 Priority levels of Low, Normal, High, Critical
- Basic round-robin scheduler
- Full register state preservation for context switching

### 4. Interrupt System
- Interrupt Descriptor Table (IDT) for complete interrupt handling
- x86 exceptions (division by zero, page fault, etc.)
- Timer and keyboard interrupt support (Hardware if drivers support)
- Software interrupt 0x80 for system call interface

### 5. File System
- Basic FAT12 filesystem implementation
- File Operations: touch, cat, rm
- Directory Operations: mkdir, rmdir
- Working directory support: pwd
- **Note: File system is currently in simulation mode - actual disk I/O operations are not implemented.**

### 6. Device Drivers

#### VGA Driver
- Text mode display (80x25 characters)
- Color support (16 colors)
- Cursor management
- Screen scrolling
- Character and string output functions

#### Keyboard Driver
- PS/2 keyboard input handling
- Character input buffering
- Special key support (backspace, enter)

### 8. Interactive Shell
- **Command Line Interface**: Full-featured shell with command parsing
- **Built-in Commands**:
  - `help` - Show available commands
  - `clear` - Clear screen
  - `info` - System information
  - `memory` - Memory statistics
  - `malloc`/`free` - Memory allocation
  - `paging` - Paging information
  - `memmap` - Memory map display
  - `ls`/`cat`/`touch`/`rm` - File operations
  - `mkdir`/`rmdir`/`cd`/`pwd` - Directory operations
  - `ps`/`kill`/`priority` - Process management
  - `syscall` - System call interface

### 9. Library Functions
- **String Operations**: strlen, strcpy, strcat, strcmp, strncpy, memset, memcpy
- **Memory Utilities**: Memory manipulation and comparison functions

## File Structure

```
oskernel/
├── arch/x86/           # x86 architecture specific code
│   ├── boot.asm        # Bootloader
│   ├── interrupt_asm.asm
│   ├── paging_asm_simple.asm
│   └── syscall_asm.asm
├── drivers/            # Device drivers
│   ├── vga/           # VGA display driver
│   └── keyboard/      # Keyboard driver
├── fs/                # File system
│   ├── filesystem.c
│   └── filesystem.h
├── kernel/            # Core kernel
│   ├── kernel.c       # Main kernel and shell
│   ├── interrupt.c    # Interrupt handling
│   ├── memory.c       # Memory management
│   ├── syscall.c      # System call implementation
│   └── process/       # Process management
├── lib/               # Library functions
│   ├── string.c
│   └── string.h
├── linker.ld          # Linker script
└── Makefile          # Build system
```

## Running the Kernel

1. Build the kernel: `make cleean && make all`
2. Run in QEMU: `make run`
3. The kernel will boot and present an interactive shell

## Technical Specifications

- **Memory Layout**: 
  - Kernel starts at 0x100000 (1MB)
  - Kernel heap at 0x200000 (2MB)
  - User space starts at 0x400000 (4MB)
- **Page Size**: 4KB
- **Maximum Processes**: 64
- **File System**: FAT12 with 512-byte sectors
- **Display**: VGA text mode 80x25
- **Interrupts**: x86 exception handling + timer/keyboard