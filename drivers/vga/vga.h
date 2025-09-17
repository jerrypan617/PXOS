#ifndef VGA_H
#define VGA_H

#include <stdint.h>

// VGA 文本模式常量
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

// 颜色定义
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_YELLOW = 14,  // 与LIGHT_BROWN相同
    VGA_COLOR_WHITE = 15,
};

// 函数声明
void vga_init(void);
void vga_clear(void);
void vga_putchar(char c);
void vga_putstr(const char* str);
void vga_set_color(enum vga_color fg, enum vga_color bg);
void vga_putchar_at(char c, enum vga_color fg, enum vga_color bg, uint8_t x, uint8_t y);
void vga_putstr_at(const char* str, enum vga_color fg, enum vga_color bg, uint8_t x, uint8_t y);
void vga_scroll(void);
void vga_putnum(int num);
void vga_puthex(uint32_t num);
void vga_update_cursor(void);
void vga_hide_cursor(void);
void vga_show_cursor(void);

#endif
