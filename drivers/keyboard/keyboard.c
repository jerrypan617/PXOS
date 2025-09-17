#include "keyboard.h"
#include "../vga/vga.h"
#include "../../lib/string.h"

// 全局键盘状态
static keyboard_state_t keyboard_state = {0};

// 输入缓冲区
static char input_buffer[INPUT_BUFFER_SIZE];
static int buffer_head = 0;
static int buffer_tail = 0;
static int buffer_count = 0;

// 扫描码到字符的映射表
static const char scancode_map[128] = {
    0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,   0,   // 0x00-0x0F
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0,   0,   'a', 's', // 0x10-0x1F
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', 0,   0,   0,   'z', 'x', 'c', 'v', // 0x20-0x2F
    'b', 'n', 'm', ',', '.', '/', 0,   0,   0,   ' ', 0,   0,   0,   0,   0,   0,   // 0x30-0x3F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 0x40-0x4F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 0x50-0x5F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 0x60-0x6F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0    // 0x70-0x7F
};

// Shift键映射表
static const char shift_map[128] = {
    0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,   0,   // 0x00-0x0F
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,   0,   'A', 'S', // 0x10-0x1F
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', 0,   0,   0,   'Z', 'X', 'C', 'V', // 0x20-0x2F
    'B', 'N', 'M', '<', '>', '?', 0,   0,   0,   ' ', 0,   0,   0,   0,   0,   0,   // 0x30-0x3F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 0x40-0x4F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 0x50-0x5F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 0x60-0x6F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0    // 0x70-0x7F
};

// 初始化键盘
void keyboard_init(void) {
    // 清零键盘状态
    memset(&keyboard_state, 0, sizeof(keyboard_state));
    
    // 清零输入缓冲区
    memset(input_buffer, 0, INPUT_BUFFER_SIZE);
    buffer_head = 0;
    buffer_tail = 0;
    buffer_count = 0;
}

// 前向声明
static void process_scancode(uint8_t scancode);

// 键盘中断处理程序
void keyboard_interrupt_handler(void) {
    // 读取键盘状态
    uint8_t status = 0;
    __asm__ volatile("inb %1, %0" : "=a"(status) : "Nd"(KEYBOARD_STATUS_PORT));
    
    // 检查是否有数据可读
    if (!(status & KEYBOARD_STATUS_OUTPUT_FULL)) {
        return;
    }
    
    // 读取扫描码
    uint8_t scancode = 0;
    __asm__ volatile("inb %1, %0" : "=a"(scancode) : "Nd"(KEYBOARD_DATA_PORT));
    
    // 处理扫描码
    process_scancode(scancode);
}

// 处理扫描码
static void process_scancode(uint8_t scancode) {
    // 检查是否是按键释放
    bool key_released = (scancode & KEY_RELEASED) != 0;
    uint8_t key_code = scancode & ~KEY_RELEASED;
    
    // 处理修饰键
    switch (key_code) {
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            keyboard_state.shift_pressed = !key_released;
            break;
            
        case KEY_LCTRL:
            keyboard_state.ctrl_pressed = !key_released;
            break;
            
        case KEY_LALT:
            keyboard_state.alt_pressed = !key_released;
            break;
            
        case KEY_CAPS_LOCK:
            if (!key_released) {
                keyboard_state.caps_lock = !keyboard_state.caps_lock;
            }
            break;
            
        case KEY_ENTER:
            if (!key_released) {
                // 回车键
                if (buffer_count < INPUT_BUFFER_SIZE) {
                    input_buffer[buffer_tail] = '\n';
                    buffer_tail = (buffer_tail + 1) % INPUT_BUFFER_SIZE;
                    buffer_count++;
                }
            }
            break;
            
        case KEY_BACKSPACE:
            if (!key_released) {
                // 退格键
                if (buffer_count < INPUT_BUFFER_SIZE) {
                    input_buffer[buffer_tail] = '\b';
                    buffer_tail = (buffer_tail + 1) % INPUT_BUFFER_SIZE;
                    buffer_count++;
                }
            }
            break;
            
        default:
            // 处理普通按键
            if (!key_released) {
                char ch = scancode_to_char(key_code, keyboard_state.shift_pressed, keyboard_state.caps_lock);
                if (ch != 0) {
                    // 将字符添加到缓冲区
                    if (buffer_count < INPUT_BUFFER_SIZE) {
                        input_buffer[buffer_tail] = ch;
                        buffer_tail = (buffer_tail + 1) % INPUT_BUFFER_SIZE;
                        buffer_count++;
                    }
                }
            }
            break;
    }
}

// 扫描码转字符
char scancode_to_char(uint8_t scancode, bool shift_pressed, bool caps_lock) {
    if (scancode >= 128) {
        return 0;
    }
    
    char ch = 0;
    
    // 检查是否是特殊键
    if (is_special_key(scancode)) {
        return 0;
    }
    
    // 根据Shift状态选择映射表
    if (shift_pressed) {
        ch = shift_map[scancode];
    } else {
        ch = scancode_map[scancode];
    }
    
    // 处理Caps Lock
    if (caps_lock && ch >= 'a' && ch <= 'z') {
        ch = ch - 'a' + 'A';
    } else if (caps_lock && ch >= 'A' && ch <= 'Z') {
        ch = ch - 'A' + 'a';
    }
    
    return ch;
}

// 检查是否是特殊键
bool is_special_key(uint8_t scancode) {
    switch (scancode) {
        case KEY_ESC:
        case KEY_TAB:
        case KEY_CAPS_LOCK:
        case KEY_LSHIFT:
        case KEY_RSHIFT:
        case KEY_LCTRL:
        case KEY_LALT:
            return true;
        default:
            return false;
    }
}

// 获取字符（非阻塞）
char keyboard_get_char(void) {
    if (buffer_count == 0) {
        return 0;
    }
    
    char ch = input_buffer[buffer_head];
    buffer_head = (buffer_head + 1) % INPUT_BUFFER_SIZE;
    buffer_count--;
    
    return ch;
}

// 检查是否有输入
bool keyboard_has_input(void) {
    return buffer_count > 0;
}

// 清空输入缓冲区
void keyboard_clear_buffer(void) {
    buffer_head = 0;
    buffer_tail = 0;
    buffer_count = 0;
    memset(input_buffer, 0, INPUT_BUFFER_SIZE);
}

// 读取字符（阻塞）
char keyboard_read_char(void) {
    while (!keyboard_has_input()) {
        // 等待输入
        __asm__ volatile("hlt");
    }
    return keyboard_get_char();
}

// 等待输入
void keyboard_wait_for_input(void) {
    while (!keyboard_has_input()) {
        __asm__ volatile("hlt");
    }
}
