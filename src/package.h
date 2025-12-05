#ifndef PACKAGES_H
#define PACKAGES_H

typedef unsigned long size_t;

void writeOut(const char *s);
void writeOutNum(long num);
void BreakLine(int index);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
int startsWith(const char *str, const char *prefix);
void exit(void);
char *readLine(char *buf, size_t bufSize);

#endif