#include "../package.h"
#include "uart.h"
#include "drivers/ps2Keyboard.h"

// Check if UART has data
static int uart_has_data(void) {
    return !(*UART0_FR & UART0_FR_RXFE);
}

// Get a character from either UART or PS/2 keyboard (non-blocking check, blocking wait)
static char getchar_any(void) {
    while (1) {
        // Check UART first
        if (uart_has_data()) {
            return (char)(*UART0_DR & 0xFF);
        }
        // Check PS/2 keyboard (Scancode Set 2)
        if (ps2_has_key()) {
            unsigned char scancode = ps2_get_scancode();
            
            // 0xF0 = key release prefix in Set 2
            if (scancode == 0xF0) {
                release_next = 1;
                continue;
            }
            
            // Handle key release
            if (release_next) {
                release_next = 0;
                // Check if shift was released
                if (scancode == 0x12 || scancode == 0x59) {
                    shift_pressed = 0;
                }
                continue;
            }
            
            // Check for shift press (Left Shift = 0x12, Right Shift = 0x59)
            if (scancode == 0x12 || scancode == 0x59) {
                shift_pressed = 1;
                continue;
            }
            
            // Convert to ASCII using Set 2 tables
            char c = shift_pressed ? 
                scancode_set2_shift[scancode] : 
                scancode_set2[scancode];
            
            if (c != 0) return c;
        }
    }
}

char *readLine(char *buf, size_t bufSize) {
    size_t len = 0;
    
    // Initialize PS/2 keyboard
    ps2_init();
    
    while (1) {
        char c = getchar_any();

        if (c == 3 || c == 27) {  // Ctrl+C or Escape
            writeOut("^C\n");
            exit();  // Shut down the kernel
        }

        if (c == '\n' || c == '\r') {
            writeOut("\n");
            buf[len] = '\0';   
            return buf;
        }
    
        // BACKSPACE
        if (c == '\b' || c == 127) {
            if (len > 0) {
                len--;
                writeOut("\b \b");
            }
            continue;
        }
    
        // normal characters
        if (len < bufSize - 1 && c >= 32 && c < 127) {
            buf[len++] = c;
            // Echo character
            char echo[2] = {c, '\0'};
            writeOut(echo);
        }
    }
}