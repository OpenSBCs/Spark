#include "uart.h"
#include "../package.h"

// writeOut

void writeOut(const char *s) {
    while (*s) {
        while (*UART0_FR & UART0_FR_TXFF);
        *UART0_DR = *s++;
    }
}

// writeOutNum

void writeOutNum(long num) {
    char buffer[20];
    int i = 0;

    if (num == 0) {
        return;
    }
    
    if (num < 0) {
        num = -num;
    }
    
    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }
    for (int j = i - 1; j >= 0; j--) {
        char c[2] = {buffer[j], '\0'};
        writeOut(c);
    }
}

// breakline

void BreakLine(int index) {
    for (int i = 0; i < index; i++) {
        writeOut("\n");
    }
}

// strcmp - compare two strings

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

// strncmp - compare first n characters of two strings

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

// startsWith - check if string starts with prefix

int startsWith(const char *str, const char *prefix) {
    while (*prefix) {
        if (*str++ != *prefix++) return 0;
    }
    return 1;
}