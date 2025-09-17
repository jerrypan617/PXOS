; interrupt_asm.asm - 中断处理程序汇编入口点
[bits 32]

; 外部C函数声明
extern interrupt_handler_common
extern exception_handler_common
extern timer_handler
extern keyboard_handler

; 通用中断处理程序入口点
global interrupt_handler_0
global interrupt_handler_1
global interrupt_handler_2
global interrupt_handler_3
global interrupt_handler_4
global interrupt_handler_5
global interrupt_handler_6
global interrupt_handler_7
global interrupt_handler_8
global interrupt_handler_9
global interrupt_handler_10
global interrupt_handler_11
global interrupt_handler_12
global interrupt_handler_13
global interrupt_handler_14
global interrupt_handler_15
global interrupt_handler_16
global interrupt_handler_17
global interrupt_handler_18
global interrupt_handler_19
global interrupt_handler_32
global interrupt_handler_33

; 宏定义：保存寄存器
%macro SAVE_REGS 0
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp
    ; 在保护模式下，段寄存器由CPU自动处理
%endmacro

; 宏定义：恢复寄存器
%macro RESTORE_REGS 0
    pop ebp
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
%endmacro

; 通用中断处理程序模板
%macro INTERRUPT_HANDLER 1
interrupt_handler_%1:
    cli                     ; 禁用中断
    SAVE_REGS               ; 保存寄存器
    
    ; 调用C处理程序
    call exception_handler_common
    
    RESTORE_REGS            ; 恢复寄存器
    iret                    ; 从中断返回
%endmacro

; 生成所有异常处理程序
INTERRUPT_HANDLER 0   ; 除零异常
INTERRUPT_HANDLER 1   ; 调试异常
INTERRUPT_HANDLER 2   ; NMI
INTERRUPT_HANDLER 3   ; 断点异常
INTERRUPT_HANDLER 4   ; 溢出异常
INTERRUPT_HANDLER 5   ; 边界检查异常
INTERRUPT_HANDLER 6   ; 无效操作码异常
INTERRUPT_HANDLER 7   ; 设备不可用异常
INTERRUPT_HANDLER 8   ; 双重故障
INTERRUPT_HANDLER 9   ; 协处理器段越界
INTERRUPT_HANDLER 10  ; 无效TSS
INTERRUPT_HANDLER 11  ; 段不存在
INTERRUPT_HANDLER 12  ; 堆栈故障
INTERRUPT_HANDLER 13  ; 一般保护故障
INTERRUPT_HANDLER 14  ; 页故障
INTERRUPT_HANDLER 15  ; 保留
INTERRUPT_HANDLER 16  ; FPU错误
INTERRUPT_HANDLER 17  ; 对齐检查
INTERRUPT_HANDLER 18  ; 机器检查
INTERRUPT_HANDLER 19  ; SIMD FPU错误

; 可屏蔽中断处理程序
interrupt_handler_32:  ; 定时器中断
    cli
    SAVE_REGS
    call timer_handler
    RESTORE_REGS
    iret

interrupt_handler_33:  ; 键盘中断
    cli
    SAVE_REGS
    call keyboard_handler
    RESTORE_REGS
    iret
