#ifndef _STRING_H_
#define _STRING_H_

#include <types.h>

void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *dst, int c, size_t n);
size_t strlen(const char *s);
char *strcpy(char *dst, const char *src);
const char *strchr(const char *s, int c);
int strcmp(const char *p, const char *q);
char *strcat(char *dest, const char *src);
char *strstr(const char *haystack, const char *needle);
void *memmove(void *dest, const void *src, size_t n);

void replace_substr(char *str, const char *old_sub, const char *new_sub);
static int is_whitespace(char c);
void trim_inplace(char *str);

#endif
