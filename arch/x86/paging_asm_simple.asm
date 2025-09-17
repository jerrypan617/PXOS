; paging_asm_simple.asm - Simplified paging operations
; Assembly functions for memory management

section .text
    global load_page_directory
    global enable_paging_asm
    global disable_paging_asm
    global invalidate_tlb_asm
    global invalidate_page_asm

; Load page directory into CR3
; void load_page_directory(uint32_t page_dir)
load_page_directory:
    push ebp
    mov ebp, esp
    
    mov eax, [ebp + 8]    ; Get page directory address
    mov cr3, eax          ; Load page directory into CR3
    
    pop ebp
    ret

; Enable paging by setting CR0.PG bit
enable_paging_asm:
    push ebp
    mov ebp, esp
    
    mov eax, cr0
    or eax, 0x80000000    ; Set PG bit (bit 31)
    mov cr0, eax
    
    pop ebp
    ret

; Disable paging by clearing CR0.PG bit
disable_paging_asm:
    push ebp
    mov ebp, esp
    
    mov eax, cr0
    and eax, 0x7FFFFFFF   ; Clear PG bit (bit 31)
    mov cr0, eax
    
    pop ebp
    ret

; Invalidate entire TLB
invalidate_tlb_asm:
    push ebp
    mov ebp, esp
    
    mov eax, cr3
    mov cr3, eax          ; Reload CR3 to invalidate TLB
    
    pop ebp
    ret

; Invalidate specific page in TLB
; void invalidate_page_asm(uint32_t virtual_addr)
invalidate_page_asm:
    push ebp
    mov ebp, esp
    
    ; Simple approach - just reload CR3
    mov eax, cr3
    mov cr3, eax          ; Reload CR3 to invalidate TLB
    
    pop ebp
    ret

; Note: get_current_page_directory is implemented in C (memory.c)

; Check if paging is enabled
; bool is_paging_enabled(void)
global is_paging_enabled
is_paging_enabled:
    push ebp
    mov ebp, esp
    
    mov eax, cr0
    and eax, 0x80000000   ; Check PG bit
    shr eax, 31           ; Move bit to position 0
    
    pop ebp
    ret
