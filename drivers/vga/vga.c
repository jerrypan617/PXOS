#include "vga.h"
#include "../../lib/string.h"
#include <stddef.h>

// Current cursor position
static uint8_t terminal_row = 0;
static uint8_t terminal_column = 0;
static uint8_t terminal_color = 0x07; // Default white text on black background

// Create VGA entry
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

// Update hardware cursor position
void vga_update_cursor(void) {
    uint16_t pos = terminal_row * VGA_WIDTH + terminal_column;
    
    // Send high byte of cursor position
    __asm__ volatile("outb %%al, %%dx" : : "a"((uint8_t)0x0E), "d"((uint16_t)0x3D4));
    __asm__ volatile("outb %%al, %%dx" : : "a"((uint8_t)((pos >> 8) & 0xFF)), "d"((uint16_t)0x3D5));
    
    // Send low byte of cursor position
    __asm__ volatile("outb %%al, %%dx" : : "a"((uint8_t)0x0F), "d"((uint16_t)0x3D4));
    __asm__ volatile("outb %%al, %%dx" : : "a"((uint8_t)(pos & 0xFF)), "d"((uint16_t)0x3D5));
}

// Hide cursor
void vga_hide_cursor(void) {
    uint16_t port = 0x3D4;
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x0A), "Nd"(port));
    port = 0x3D5;
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"(port));
}

// Show cursor
void vga_show_cursor(void) {
    vga_update_cursor();
}

// Initialize VGA
void vga_init(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
}

// Clear screen
void vga_clear(void) {
    uint16_t* terminal_buffer = (uint16_t*) VGA_MEMORY;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
    
    // Display PXOS kernel ASCII art
    const char* pxos_art[] = {
        "         _    _      _             _           _        ",
        "        /\\ \\/_/\\    /\\ \\          /\\ \\        / /\\      ",
        "       /  \\ \\ \\ \\   \\ \\_\\        /  \\ \\      / /  \\     ",
        "      / /\\ \\ \\ \\ \\__/ / /       / /\\ \\ \\    / / /\\ \\__  ",
        "     / / /\\ \\_\\ \\__ \\/_/       / / /\\ \\ \\  / / /\\ \\___\\ ",
        "    / / /_/ / /\\/_/\\__/\\      / / /  \\ \\_\\ \\ \\ \\ \\/___/ ",
        "   / / /__\\/ /  _/\\/__\\ \\    / / /   / / /  \\ \\ \\       ",
        "  / / /_____/  / _/_/\\ \\ \\  / / /   / / /    \\ \\ \\      ",
        " / / /        / / /   \\ \\ \\/ / /___/ / /_/\\__/ / /      ",
        "/ / /        / / /    /_/ / / /____\\/ /\\ \\/___/ /       ",
        "\\/_/         \\/_/     \\_\\/\\/_________/  \\_____\\/        ",
        "                                                        "
    };
    
    // Calculate center position
    size_t art_height = sizeof(pxos_art) / sizeof(pxos_art[0]);
    size_t start_y = 2; // Start displaying from line 3
    
    // Use cyan color for art
    uint8_t art_color = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    
    for (size_t i = 0; i < art_height && (start_y + i) < VGA_HEIGHT; i++) {
        const char* line = pxos_art[i];
        size_t line_len = strlen(line);
        size_t start_x = (VGA_WIDTH - line_len) / 2; // Center horizontally
        
        for (size_t j = 0; j < line_len && (start_x + j) < VGA_WIDTH; j++) {
            size_t index = (start_y + i) * VGA_WIDTH + (start_x + j);
            terminal_buffer[index] = vga_entry(line[j], art_color);
        }
    }
    
    // Add separator line below art
    size_t separator_y = start_y + art_height + 1;
    if (separator_y < VGA_HEIGHT) {
        uint8_t separator_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            size_t index = separator_y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry('-', separator_color);
        }
    }
    
    // Set cursor position below art
    terminal_row = separator_y + 2;
    terminal_column = 0;
    vga_update_cursor();
}

// Set color
void vga_set_color(enum vga_color fg, enum vga_color bg) {
    terminal_color = vga_entry_color(fg, bg);
}

// Place character at specified position
void vga_putchar_at(char c, enum vga_color fg, enum vga_color bg, uint8_t x, uint8_t y) {
    const size_t index = y * VGA_WIDTH + x;
    uint16_t* terminal_buffer = (uint16_t*) VGA_MEMORY;
    terminal_buffer[index] = vga_entry(c, vga_entry_color(fg, bg));
}

// Place string at specified position
void vga_putstr_at(const char* str, enum vga_color fg, enum vga_color bg, uint8_t x, uint8_t y) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        vga_putchar_at(str[i], fg, bg, x + i, y);
    }
}

// Output character
void vga_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            vga_scroll();
        }
    } else if (c == '\b') {
        // Handle backspace character
        if (terminal_column > 0) {
            terminal_column--;
            vga_putchar_at(' ', VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK, terminal_column, terminal_row);
        }
    } else {
        // Extract foreground and background colors from terminal_color
        enum vga_color fg = (enum vga_color)(terminal_color & 0x0F);
        enum vga_color bg = (enum vga_color)((terminal_color >> 4) & 0x0F);
        vga_putchar_at(c, fg, bg, terminal_column, terminal_row);
        if (++terminal_column == VGA_WIDTH) {
            terminal_column = 0;
            if (++terminal_row == VGA_HEIGHT) {
                vga_scroll();
            }
        }
    }
    vga_update_cursor();
}

// Output string
void vga_putstr(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        vga_putchar(str[i]);
    }
}

// Scroll screen
void vga_scroll(void) {
    uint16_t* terminal_buffer = (uint16_t*) VGA_MEMORY;
    
    // Move each line up
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            const size_t next_index = (y + 1) * VGA_WIDTH + x;
            terminal_buffer[index] = terminal_buffer[next_index];
        }
    }
    
    // Clear last line
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        terminal_buffer[index] = vga_entry(' ', terminal_color);
    }
    
    terminal_row = VGA_HEIGHT - 1;
    terminal_column = 0;
}

// Output number
void vga_putnum(int num) {
    if (num == 0) {
        vga_putchar('0');
        return;
    }
    
    if (num < 0) {
        vga_putchar('-');
        num = -num;
    }
    
    char buffer[32];
    int i = 0;
    
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    // Output in reverse order
    for (int j = i - 1; j >= 0; j--) {
        vga_putchar(buffer[j]);
    }
}

// Output hexadecimal number
void vga_puthex(uint32_t num) {
    vga_putstr("0x");
    
    if (num == 0) {
        vga_putchar('0');
        return;
    }
    
    char buffer[9];
    int i = 0;
    
    while (num > 0) {
        int digit = num & 0xF;
        if (digit < 10) {
            buffer[i++] = '0' + digit;
        } else {
            buffer[i++] = 'A' + (digit - 10);
        }
        num >>= 4;
    }
    
    // Output in reverse order
    for (int j = i - 1; j >= 0; j--) {
        vga_putchar(buffer[j]);
    }
}
