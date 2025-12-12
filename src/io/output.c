#include "uart.h"
#include <drivers/graphicsDriver.h>
#include <package.h>

// Graphics mode flag (0 = UART only, 1 = Graphics + UART)
static int graphics_enabled = 0;

// Software division helper for ARM (no hardware divider)
static unsigned int soft_div(unsigned int n, unsigned int d) {
    unsigned int q = 0;
    unsigned int r = 0;
    for (int i = 31; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) {
            r -= d;
            q |= (1U << i);
        }
    }
    return q;
}

static unsigned int soft_mod(unsigned int n, unsigned int d) {
    unsigned int r = 0;
    for (int i = 31; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) {
            r -= d;
        }
    }
    return r;
}

// Initialize graphics mode
void initGraphics(void) {
    gfx_init();
    graphics_enabled = 1;
}

// ANSI escape sequence parser state
static int ansi_state = 0;  // 0=normal, 1=got ESC, 2=got [, 3=got ?
static int ansi_params[8];
static int ansi_param_count = 0;
static int ansi_current_param = 0;

// Reset ANSI parser
static void ansi_reset(void) {
    ansi_state = 0;
    ansi_param_count = 0;
    ansi_current_param = 0;
    for (int i = 0; i < 8; i++) ansi_params[i] = 0;
}

// Process completed ANSI sequence
static void ansi_execute(char cmd) {
    // Store final parameter
    if (ansi_param_count < 8) {
        ansi_params[ansi_param_count++] = ansi_current_param;
    }
    
    switch (cmd) {
        case 'H':  // Cursor position (row;col)
        case 'f':
            {
                int row = (ansi_param_count > 0 && ansi_params[0] > 0) ? ansi_params[0] - 1 : 0;
                int col = (ansi_param_count > 1 && ansi_params[1] > 0) ? ansi_params[1] - 1 : 0;
                gfx_set_cursor(col, row);
            }
            break;
        case 'J':  // Erase display
            if (ansi_params[0] == 2) {
                gfx_clear();  // Clear entire screen
            }
            break;
        case 'K':  // Erase line
            gfx_clear_to_eol();  // Clear to end of line
            break;
        case 'm':  // SGR - Select Graphic Rendition
            for (int i = 0; i < ansi_param_count; i++) {
                switch (ansi_params[i]) {
                    case 0:  // Reset
                        gfx_set_colors(COLOR_WHITE, COLOR_BLACK);
                        break;
                    case 7:  // Reverse video
                        gfx_set_colors(COLOR_BLACK, COLOR_WHITE);
                        break;
                    case 27: // Normal video (not reversed)
                        gfx_set_colors(COLOR_WHITE, COLOR_BLACK);
                        break;
                }
            }
            break;
        case 'l':  // Reset mode (e.g., hide cursor ?25l)
        case 'h':  // Set mode (e.g., show cursor ?25h)
            // Cursor visibility - ignore for now
            break;
    }
    ansi_reset();
}

// writeOut - outputs to both UART and graphics (if enabled)
// Parses ANSI escape sequences for graphics output

void writeOut(const char *s) {
    while (*s) {
        char c = *s++;
        
        // Always write to UART (it handles ANSI natively)
        while (*UART0_FR & UART0_FR_TXFF);
        *UART0_DR = (unsigned int)(unsigned char)c;

        // Handle graphics with ANSI parsing
        if (graphics_enabled) {
            switch (ansi_state) {
                case 0:  // Normal state
                    if (c == '\033') {
                        ansi_state = 1;  // Got ESC
                    } else {
                        gfx_putchar((unsigned char)c);
                    }
                    break;
                    
                case 1:  // Got ESC
                    if (c == '[') {
                        ansi_state = 2;  // Got CSI
                        ansi_param_count = 0;
                        ansi_current_param = 0;
                        for (int i = 0; i < 8; i++) ansi_params[i] = 0;
                    } else {
                        // Not a CSI sequence, output as-is
                        gfx_putchar('\033');
                        gfx_putchar((unsigned char)c);
                        ansi_state = 0;
                    }
                    break;
                    
                case 2:  // Inside CSI sequence
                    if (c == '?') {
                        ansi_state = 3;  // Private mode sequence
                    } else if (c >= '0' && c <= '9') {
                        ansi_current_param = ansi_current_param * 10 + (c - '0');
                    } else if (c == ';') {
                        if (ansi_param_count < 8) {
                            ansi_params[ansi_param_count++] = ansi_current_param;
                        }
                        ansi_current_param = 0;
                    } else if (c >= 'A' && c <= 'Z') {
                        ansi_execute(c);
                    } else if (c >= 'a' && c <= 'z') {
                        ansi_execute(c);
                    } else {
                        // Unknown, reset
                        ansi_reset();
                    }
                    break;
                    
                case 3:  // Private mode (ESC[?)
                    if (c >= '0' && c <= '9') {
                        ansi_current_param = ansi_current_param * 10 + (c - '0');
                    } else if (c == 'l' || c == 'h') {
                        ansi_execute(c);
                    } else {
                        ansi_reset();
                    }
                    break;
            }
        }
    }
}

// writeOutNum

void writeOutNum(long num) {
    char buffer[20];
    int i = 0;

    if (num == 0) {
        writeOut("0");
        return;
    }

    if (num < 0) {
        writeOut("-");
        num = -num;
    }

    while (num > 0) {
        buffer[i++] = soft_mod(num, 10) + '0';
        num = soft_div(num, 10);
    }
    for (int j = i - 1; j >= 0; j--) {
        char c[2] = {buffer[j], '\0'};
        writeOut(c);
    }
}

// breakline

void BreakLine(int times) {
    for (int i = 0; i < times; i++) {
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

// strcpy - copy string from src to dest

char *strcpy(char *dest, const char *src) {
    char *original = dest;
    while ((*dest++ = *src++));
    return original;
}
