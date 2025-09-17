#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

// 键盘端口
#define KEYBOARD_DATA_PORT    0x60
#define KEYBOARD_STATUS_PORT  0x64
#define KEYBOARD_COMMAND_PORT 0x64

// 键盘状态位
#define KEYBOARD_STATUS_OUTPUT_FULL  0x01
#define KEYBOARD_STATUS_INPUT_FULL   0x02

// 特殊键码
#define KEY_ENTER     0x1C
#define KEY_BACKSPACE 0x0E
#define KEY_ESC       0x01
#define KEY_TAB       0x0F
#define KEY_CAPS_LOCK 0x3A
#define KEY_LSHIFT    0x2A
#define KEY_RSHIFT    0x36
#define KEY_LCTRL     0x1D
#define KEY_LALT      0x38
#define KEY_SPACE     0x39

// 键状态
#define KEY_RELEASED  0x80

// 输入缓冲区大小
#define INPUT_BUFFER_SIZE 256

// 键盘状态结构
typedef struct {
    bool shift_pressed;
    bool ctrl_pressed;
    bool alt_pressed;
    bool caps_lock;
    bool num_lock;
    bool scroll_lock;
} keyboard_state_t;

// 函数声明
void keyboard_init(void);
void keyboard_interrupt_handler(void);
char keyboard_get_char(void);
bool keyboard_has_input(void);
void keyboard_clear_buffer(void);
char keyboard_read_char(void);
void keyboard_wait_for_input(void);

// 扫描码转换函数
char scancode_to_char(uint8_t scancode, bool shift_pressed, bool caps_lock);
bool is_special_key(uint8_t scancode);

#endif
