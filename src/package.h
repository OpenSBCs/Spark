#ifndef PACKAGES_H
#define PACKAGES_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;

typedef unsigned long size_t;

char *strcpy(char *dest, const char *src);
void initGraphics(void);
void writeOut(const char *s);
void writeOutNum(long num);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
int startsWith(const char *str, const char *prefix);
void exit(void);
int readline(char *buf, size_t bufSize);

#endif