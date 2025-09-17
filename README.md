```
         _    _      _             _           _        
        /\ \/_/\    /\ \          /\ \        / /\      
       /  \ \ \ \   \ \_\        /  \ \      / /  \     
      / /\ \ \ \ \__/ / /       / /\ \ \    / / /\ \__  
     / / /\ \_\ \__ \/_/       / / /\ \ \  / / /\ \___\ 
    / / /_/ / /\/_/\__/\      / / /  \ \_\ \ \ \ \/___/ 
   / / /__\/ /  _/\/__\ \    / / /   / / /  \ \ \       
  / / /_____/  / _/_/\ \ \  / / /   / / /    \ \ \      
 / / /        / / /   \ \ \/ / /___/ / /_/\__/ / /      
/ / /        / / /    /_/ / / /____\/ /\ \/___/ /       
\/_/         \/_/     \_\/\/_________/  \_____\/        
                                                        
```

# PXOS Kernel

A 32-bit x86 operating system kernel written in C and assembly, featuring a modular architecture with process management, memory management, filesystem support, and an interactive shell.

## Architecture

- **Target Architecture**: x86-32 (32-bit protected mode)
- **Bootloader**: Custom assembly bootloader that loads the kernel from floppy disk
- **Memory Model**: Protected mode with paging support
- **Display**: VGA text mode (80x25)

## Core Components

| Component | Description | Key Features |
|-----------|-------------|--------------|
| **Boot System** | Assembly bootloader | Real mode → Protected mode, GDT setup |
| **Memory Management** | Virtual memory & paging | 4KB pages, heap allocation, memory tracking |
| **Process Management** | Process scheduling | PCB, 5 states, 4 priorities, context switching |
| **Interrupt System** | Hardware/software interrupts | IDT, x86 exceptions, system calls (int 0x80) |
| **File System** | FAT12 filesystem | File/directory ops, simulation mode* |
| **Device Drivers** | Hardware interfaces | VGA display, PS/2 keyboard |
| **Interactive Shell** | Command interface | 20+ built-in commands, full CLI |
| **Library Functions** | Utility functions | String/memory operations |

*File system is in simulation mode - no actual disk I/O

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