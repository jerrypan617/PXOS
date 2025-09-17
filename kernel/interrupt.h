#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdint.h>

// 中断描述符表项结构
typedef struct {
    uint16_t offset_low;    // 中断处理程序地址的低16位
    uint16_t selector;      // 代码段选择子
    uint8_t zero;           // 必须为0
    uint8_t type_attr;      // 类型和属性
    uint16_t offset_high;   // 中断处理程序地址的高16位
} __attribute__((packed)) idt_entry_t;

// IDT描述符结构
typedef struct {
    uint16_t limit;         // IDT大小-1
    uint32_t base;          // IDT基地址
} __attribute__((packed)) idt_descriptor_t;

// 中断处理程序函数指针类型
typedef void (*interrupt_handler_t)(void);

// 中断向量号定义
#define INT_DIVIDE_BY_ZERO     0
#define INT_DEBUG              1
#define INT_NMI                2
#define INT_BREAKPOINT         3
#define INT_OVERFLOW           4
#define INT_BOUND_RANGE        5
#define INT_INVALID_OPCODE     6
#define INT_DEVICE_NOT_AVAIL   7
#define INT_DOUBLE_FAULT       8
#define INT_COPROCESSOR_SEG    9
#define INT_INVALID_TSS        10
#define INT_SEGMENT_NOT_PRESENT 11
#define INT_STACK_FAULT        12
#define INT_GENERAL_PROTECTION 13
#define INT_PAGE_FAULT         14
#define INT_FPU_ERROR          16
#define INT_ALIGNMENT_CHECK    17
#define INT_MACHINE_CHECK      18
#define INT_SIMD_FPU_ERROR     19

// 可屏蔽中断向量
#define INT_TIMER              32
#define INT_KEYBOARD           33

// 中断属性定义
#define IDT_ATTR_PRESENT       0x80
#define IDT_ATTR_DPL_0         0x00
#define IDT_ATTR_DPL_1         0x20
#define IDT_ATTR_DPL_2         0x40
#define IDT_ATTR_DPL_3         0x60
#define IDT_ATTR_32BIT_INT     0x0E
#define IDT_ATTR_32BIT_TRAP    0x0F

// 函数声明
void idt_init(void);
void idt_set_entry(uint8_t index, uint32_t handler, uint16_t selector, uint8_t attributes);
void idt_load(void);
void interrupt_handler_common(void);
void exception_handler_common(void);

// 异常处理程序
void divide_by_zero_handler(void);
void debug_handler(void);
void nmi_handler(void);
void breakpoint_handler(void);
void overflow_handler(void);
void bound_range_handler(void);
void invalid_opcode_handler(void);
void device_not_available_handler(void);
void double_fault_handler(void);
void coprocessor_segment_handler(void);
void invalid_tss_handler(void);
void segment_not_present_handler(void);
void stack_fault_handler(void);
void general_protection_handler(void);
void page_fault_handler(void);
void fpu_error_handler(void);
void alignment_check_handler(void);
void machine_check_handler(void);
void simd_fpu_error_handler(void);

// 中断处理程序
void timer_handler(void);
void keyboard_handler(void);

#endif
