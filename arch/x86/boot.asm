; boot.asm - Bootloader for loading a C kernel from floppy disk
[bits 16]
[org 0x7C00]

; Initialize segment registers
_start:
    cli                 ; Disable interrupts
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00      ; Set stack pointer below bootloader
    
    ; Display boot message
    mov si, boot_msg
    call print_string

; Load kernel from floppy disk
load_kernel:
    ; Reset floppy disk system
    mov ah, 0x00        ; Reset disk system
    mov dl, 0x00        ; Drive A (floppy)
    int 0x13
    jc disk_error       ; If reset failed, show error
    
    ; Load kernel (read more sectors to accommodate larger kernel)
    mov ah, 0x02        ; BIOS read sector function
    mov al, 80          ; Number of sectors to read (80 sectors = 40KB, enough for kernel with process management)
    mov ch, 0           ; Cylinder 0
    mov cl, 2           ; Start from sector 2 (sector 1 is bootloader)
    mov dh, 0           ; Head 0
    mov dl, 0x00        ; First floppy disk
    mov bx, 0x1000      ; Load kernel at 0x10000
    mov es, bx
    xor bx, bx
    int 0x13            ; BIOS disk interrupt
    jc disk_error       ; Jump if carry flag set (error)
    
    ; Verify we loaded something
    cmp ax, 80          ; Check if we read the expected number of sectors
    jne disk_error

; Switch to protected mode
switch_to_pm:
    cli
    lgdt [gdt_descriptor] ; Load Global Descriptor Table
    mov eax, cr0
    or eax, 1           ; Set protected mode bit
    mov cr0, eax
    jmp 0x08:protected_mode ; Far jump to flush CS

[bits 32]
protected_mode:
    mov ax, 0x10        ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000    ; Set stack pointer for protected mode

; Jump to kernel
    call 0x10000        ; Call kernel entry point
    jmp $               ; Infinite loop if kernel returns

; Disk error handling
disk_error:
    mov si, error_msg
    call print_string
    mov si, error_msg2
    call print_string
    jmp halt

; Print string function
print_string:
    lodsb
    cmp al, 0
    je print_done
    mov ah, 0x0E        ; BIOS teletype output
    mov bh, 0           ; Page 0
    mov bl, 0x0C        ; Red color
    int 0x10
    jmp print_string
print_done:
    ret

halt:
    jmp $

; Data
boot_msg db 'Booting from floppy disk...', 13, 10, 0
error_msg db 'Boot Error: Cannot read from floppy disk!', 13, 10, 0
error_msg2 db 'Please check floppy disk and restart.', 13, 10, 0

; Global Descriptor Table
gdt_start:
    ; Null descriptor
    dd 0
    dd 0
    ; Code segment descriptor
    dw 0xFFFF           ; Limit (0-15)
    dw 0x0000           ; Base (0-15)
    db 0x00             ; Base (16-23)
    db 0x9A             ; Access byte: present, ring 0, code, executable
    db 0xCF             ; Granularity: 4KB, 32-bit
    db 0x00             ; Base (24-31)
    ; Data segment descriptor
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92             ; Access byte: present, ring 0, data, writable
    db 0xCF
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; Boot sector signature
times 510-($-$$) db 0
dw 0xAA55