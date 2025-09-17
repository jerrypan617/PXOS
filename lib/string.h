#ifndef STRING_H
#define STRING_H

#include <stddef.h>

// 字符串函数声明
size_t strlen(const char* str);
int strcmp(const char* str1, const char* str2);
int strcasecmp(const char* str1, const char* str2);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);
void* memset(void* ptr, int value, size_t num);
void* memcpy(void* dest, const void* src, size_t num);
int memcmp(const void* ptr1, const void* ptr2, size_t num);
char* strchr(const char* str, int character);
char* strrchr(const char* str, int character);
int atoi(const char* str);
void itoa(int num, char* str, int base);

#endif
