#ifndef STD_STRING_H
#define STD_STRING_H

#include <stddef.h>

size_t strlen(const char* str);
char* strcpy(char* dest, const char* src);
char* strcat(char* dest, const char* src);
int strcmp(const char* str1, const char* str2);
int strncmp(const char* str1, const char* str2, size_t n);
char* strchr(const char* str, int c);
char* strdup(const char* str);
void strncpy(char* dest, const char* src, size_t n);

#endif // STD_STRING_H