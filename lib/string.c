#include "string.h"

// 计算字符串长度
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

// 比较两个字符串
int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

int strcasecmp(const char* str1, const char* str2) {
    while (*str1 && *str2) {
        char c1 = *str1;
        char c2 = *str2;
        
        // 转换为小写进行比较
        if (c1 >= 'A' && c1 <= 'Z') {
            c1 = c1 - 'A' + 'a';
        }
        if (c2 >= 'A' && c2 <= 'Z') {
            c2 = c2 - 'A' + 'a';
        }
        
        if (c1 != c2) {
            return c1 - c2;
        }
        
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// 复制字符串
char* strcpy(char* dest, const char* src) {
    char* temp = dest;
    while ((*dest++ = *src++));
    return temp;
}

// 字符串复制（限制长度）
char* strncpy(char* dest, const char* src, size_t n) {
    char* ptr = dest;
    size_t i = 0;
    
    while (i < n && *src) {
        *ptr++ = *src++;
        i++;
    }
    
    // 如果源字符串长度小于n，用0填充剩余部分
    while (i < n) {
        *ptr++ = '\0';
        i++;
    }
    
    return dest;
}

// 连接字符串
char* strcat(char* dest, const char* src) {
    char* temp = dest;
    while (*dest) dest++;
    while ((*dest++ = *src++));
    return temp;
}

// 设置内存
void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (unsigned char)value;
    }
    return ptr;
}

// 复制内存
void* memcpy(void* dest, const void* src, size_t num) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    for (size_t i = 0; i < num; i++) {
        d[i] = s[i];
    }
    return dest;
}

// 字符串转整数
int atoi(const char* str) {
    int result = 0;
    int sign = 1;
    
    // 跳过空白字符
    while (*str == ' ' || *str == '\t' || *str == '\n') {
        str++;
    }
    
    // 处理符号
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    // 转换数字
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return sign * result;
}

// 整数转字符串
void itoa(int num, char* str, int base) {
    int i = 0;
    int is_negative = 0;
    
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    
    if (num < 0 && base == 10) {
        is_negative = 1;
        num = -num;
    }
    
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }
    
    if (is_negative) {
        str[i++] = '-';
    }
    
    str[i] = '\0';
    
    // 反转字符串
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

// 比较内存区域
int memcmp(const void* ptr1, const void* ptr2, size_t num) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;
    
    for (size_t i = 0; i < num; i++) {
        if (p1[i] < p2[i]) {
            return -1;
        } else if (p1[i] > p2[i]) {
            return 1;
        }
    }
    
    return 0;
}

// 查找字符在字符串中第一次出现的位置
char* strchr(const char* str, int character) {
    while (*str) {
        if (*str == (char)character) {
            return (char*)str;
        }
        str++;
    }
    
    if (character == '\0') {
        return (char*)str;
    }
    
    return NULL;
}

// 查找字符在字符串中最后一次出现的位置
char* strrchr(const char* str, int character) {
    char* last = NULL;
    
    while (*str) {
        if (*str == (char)character) {
            last = (char*)str;
        }
        str++;
    }
    
    if (character == '\0') {
        return (char*)str;
    }
    
    return last;
}
