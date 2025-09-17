#include "interrupt.h"
#include "../drivers/vga/vga.h"
#include "../drivers/keyboard/keyboard.h"
#include <stddef.h>

// 外部汇编处理程序声明
extern void interrupt_handler_32(void);
extern void interrupt_handler_33(void);
extern void syscall_entry(void);

// IDT表 (256个条目)
static idt_entry_t idt[256];

// IDT描述符
static idt_descriptor_t idt_desc;

// 中断处理程序表
static interrupt_handler_t interrupt_handlers[256];

// 初始化PIC
void pic_init(void) {
    // 初始化主PIC
    __asm__ volatile("movb $0x11, %al");      // ICW1: 边沿触发, 级联, 需要ICW4
    __asm__ volatile("outb %al, $0x20");      // 发送到主PIC
    __asm__ volatile("movb $0x20, %al");      // ICW2: 主PIC中断向量起始地址
    __asm__ volatile("outb %al, $0x21");      // 发送到主PIC
    __asm__ volatile("movb $0x04, %al");      // ICW3: 从PIC连接到IRQ2
    __asm__ volatile("outb %al, $0x21");      // 发送到主PIC
    __asm__ volatile("movb $0x01, %al");      // ICW4: 8086模式
    __asm__ volatile("outb %al, $0x21");      // 发送到主PIC
    
    // 初始化从PIC
    __asm__ volatile("movb $0x11, %al");      // ICW1: 边沿触发, 级联, 需要ICW4
    __asm__ volatile("outb %al, $0xA0");      // 发送到从PIC
    __asm__ volatile("movb $0x28, %al");      // ICW2: 从PIC中断向量起始地址
    __asm__ volatile("outb %al, $0xA1");      // 发送到从PIC
    __asm__ volatile("movb $0x02, %al");      // ICW3: 从PIC标识
    __asm__ volatile("outb %al, $0xA1");      // 发送到从PIC
    __asm__ volatile("movb $0x01, %al");      // ICW4: 8086模式
    __asm__ volatile("outb %al, $0xA1");      // 发送到从PIC
    
    // 启用键盘中断 (IRQ1)
    __asm__ volatile("movb $0xFD, %al");      // 只启用键盘中断
    __asm__ volatile("outb %al, $0x21");      // 发送到主PIC
}

// 初始化IDT
void idt_init(void) {
    // 初始化PIC
    pic_init();
    
    // 设置IDT描述符
    idt_desc.limit = sizeof(idt) - 1;
    idt_desc.base = (uint32_t)&idt;
    
    // 清零所有IDT条目
    for (int i = 0; i < 256; i++) {
        idt_set_entry(i, 0, 0, 0);
        interrupt_handlers[i] = NULL;
    }
    
    // 设置异常处理程序
    idt_set_entry(INT_DIVIDE_BY_ZERO, (uint32_t)divide_by_zero_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    idt_set_entry(INT_DEBUG, (uint32_t)debug_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    idt_set_entry(INT_NMI, (uint32_t)nmi_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    idt_set_entry(INT_BREAKPOINT, (uint32_t)breakpoint_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_3 | IDT_ATTR_32BIT_TRAP);
    idt_set_entry(INT_OVERFLOW, (uint32_t)overflow_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    idt_set_entry(INT_BOUND_RANGE, (uint32_t)bound_range_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    idt_set_entry(INT_INVALID_OPCODE, (uint32_t)invalid_opcode_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    idt_set_entry(INT_DEVICE_NOT_AVAIL, (uint32_t)device_not_available_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    idt_set_entry(INT_DOUBLE_FAULT, (uint32_t)double_fault_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    idt_set_entry(INT_COPROCESSOR_SEG, (uint32_t)coprocessor_segment_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    idt_set_entry(INT_INVALID_TSS, (uint32_t)invalid_tss_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    idt_set_entry(INT_SEGMENT_NOT_PRESENT, (uint32_t)segment_not_present_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    idt_set_entry(INT_STACK_FAULT, (uint32_t)stack_fault_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    idt_set_entry(INT_GENERAL_PROTECTION, (uint32_t)general_protection_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    idt_set_entry(INT_PAGE_FAULT, (uint32_t)page_fault_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    idt_set_entry(INT_FPU_ERROR, (uint32_t)fpu_error_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    idt_set_entry(INT_ALIGNMENT_CHECK, (uint32_t)alignment_check_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    idt_set_entry(INT_MACHINE_CHECK, (uint32_t)machine_check_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    idt_set_entry(INT_SIMD_FPU_ERROR, (uint32_t)simd_fpu_error_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    
    // 设置可屏蔽中断处理程序
    idt_set_entry(INT_TIMER, (uint32_t)interrupt_handler_32, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    idt_set_entry(INT_KEYBOARD, (uint32_t)interrupt_handler_33, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_0 | IDT_ATTR_32BIT_INT);
    
    // 设置系统调用中断处理程序 (0x80)
    idt_set_entry(0x80, (uint32_t)syscall_entry, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_3 | IDT_ATTR_32BIT_TRAP);
    
    // 加载IDT
    idt_load();
}

// 设置IDT条目
void idt_set_entry(uint8_t index, uint32_t handler, uint16_t selector, uint8_t attributes) {
    idt[index].offset_low = handler & 0xFFFF;
    idt[index].selector = selector;
    idt[index].zero = 0;
    idt[index].type_attr = attributes;
    idt[index].offset_high = (handler >> 16) & 0xFFFF;
}

// 加载IDT
void idt_load(void) {
    __asm__ volatile("lidt %0" : : "m"(idt_desc));
}

// 通用中断处理程序
void interrupt_handler_common(void) {
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_putstr("Unhandled interrupt occurred!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

// 通用异常处理程序
void exception_handler_common(void) {
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_putstr("Unhandled exception occurred!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

// 异常处理程序实现
void divide_by_zero_handler(void) {
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_putstr("Exception: Division by Zero!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void debug_handler(void) {
    vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    vga_putstr("Debug exception occurred.\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void nmi_handler(void) {
    vga_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
    vga_putstr("NMI (Non-Maskable Interrupt) occurred.\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void breakpoint_handler(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_putstr("Breakpoint exception occurred.\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void overflow_handler(void) {
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_putstr("Exception: Overflow!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void bound_range_handler(void) {
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_putstr("Exception: Bound Range Exceeded!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void invalid_opcode_handler(void) {
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_putstr("Exception: Invalid Opcode!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void device_not_available_handler(void) {
    vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    vga_putstr("Exception: Device Not Available!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void double_fault_handler(void) {
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_putstr("FATAL: Double Fault!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void coprocessor_segment_handler(void) {
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_putstr("Exception: Coprocessor Segment Overrun!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void invalid_tss_handler(void) {
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_putstr("Exception: Invalid TSS!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void segment_not_present_handler(void) {
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_putstr("Exception: Segment Not Present!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void stack_fault_handler(void) {
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_putstr("Exception: Stack Fault!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void general_protection_handler(void) {
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_putstr("Exception: General Protection Fault!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void page_fault_handler(void) {
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_putstr("Exception: Page Fault!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void fpu_error_handler(void) {
    vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    vga_putstr("Exception: FPU Error!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void alignment_check_handler(void) {
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_putstr("Exception: Alignment Check!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void machine_check_handler(void) {
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_putstr("FATAL: Machine Check!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void simd_fpu_error_handler(void) {
    vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    vga_putstr("Exception: SIMD FPU Error!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

// 中断处理程序实现
void timer_handler(void) {
    // 简单的定时器处理 - 不调用复杂函数
    static int timer_count = 0;
    timer_count++;
    
    // 只做简单的计数，不调用VGA函数
    // 这样可以避免在中断中调用复杂函数导致的问题
    
    // 发送中断结束信号给PIC
    __asm__ volatile("movb $0x20, %al");
    __asm__ volatile("outb %al, $0x20");
}

void keyboard_handler(void) {
    // 调用键盘驱动处理程序
    extern void keyboard_interrupt_handler(void);
    keyboard_interrupt_handler();
    
    // 发送中断结束信号给PIC
    __asm__ volatile("movb $0x20, %al");
    __asm__ volatile("outb %al, $0x20");
}
