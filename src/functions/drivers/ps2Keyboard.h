#ifndef PS2_KEYBOARD_H
#define PS2_KEYBOARD_H

// PL050 KMI (Keyboard/Mouse Interface) for VersatilePB
#define KMI0_BASE       0x10006000

#define KMI_CR          ((volatile unsigned int*)(KMI0_BASE + 0x00))  // Control
#define KMI_STAT        ((volatile unsigned int*)(KMI0_BASE + 0x04))  // Status
#define KMI_DATA        ((volatile unsigned int*)(KMI0_BASE + 0x08))  // Data
#define KMI_CLKDIV      ((volatile unsigned int*)(KMI0_BASE + 0x0C))  // Clock divisor

// Status bits
#define KMI_STAT_RXFULL  (1 << 4)  // Receive register full

// PS/2 Scancode Set 2 to ASCII - Norwegian layout
// The PL050 in QEMU uses Scancode Set 2
static const char scancode_set2[256] = {
    // 0x00 - 0x0F
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    '\t', '|',  0,
    // 0x10 - 0x1F
    0,    0,    0,    0,    0,    'q',  '1',  0,    0,    0,    'z',  's',  'a',  'w',  '2',  0,
    // 0x20 - 0x2F
    0,    'c',  'x',  'd',  'e',  '4',  '3',  0,    0,    ' ',  'v',  'f',  't',  'r',  '5',  0,
    // 0x30 - 0x3F
    0,    'n',  'b',  'h',  'g',  'y',  '6',  0,    0,    0,    'm',  'j',  'u',  '7',  '8',  0,
    // 0x40 - 0x4F
    0,    ',',  'k',  'i',  'o',  '0',  '9',  0,    0,    '.',  '-',  'l',  ';',  'p',  '+',  0,
    // 0x50 - 0x5F
    0,    0,    '\'', 0,    '[',  '\\', 0,    0,    0,    0,    '\n', ']',  0,    '\'', 0,    0,
    // 0x60 - 0x6F
    0,    '<',  0,    0,    0,    0,    '\b', 0,    0,    '1',  0,    '4',  '7',  0,    0,    0,
    // 0x70 - 0x7F
    '0',  '.',  '2',  '5',  '6',  '8',  27,   0,    0,    '+',  '3',  '-',  '*',  '9',  0,    0,
    // 0x80 - 0xFF (unused, fill with 0)
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

// PS/2 Scancode Set 2 to ASCII - Norwegian layout (SHIFTED)
static const char scancode_set2_shift[256] = {
    // 0x00 - 0x0F
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    '\t', '~',  0,
    // 0x10 - 0x1F
    0,    0,    0,    0,    0,    'Q',  '!',  0,    0,    0,    'Z',  'S',  'A',  'W',  '"',  0,
    // 0x20 - 0x2F
    0,    'C',  'X',  'D',  'E',  '$',  '#',  0,    0,    ' ',  'V',  'F',  'T',  'R',  '%',  0,
    // 0x30 - 0x3F
    0,    'N',  'B',  'H',  'G',  'Y',  '&',  0,    0,    0,    'M',  'J',  'U',  '/',  '(',  0,
    // 0x40 - 0x4F
    0,    ';',  'K',  'I',  'O',  '=',  ')',  0,    0,    ':',  '_',  'L',  ':',  'P',  '?',  0,
    // 0x50 - 0x5F
    0,    0,    '*',  0,    '{',  '`',  0,    0,    0,    0,    '\n', '}',  0,    '*',  0,    0,
    // 0x60 - 0x6F
    0,    '>',  0,    0,    0,    0,    '\b', 0,    0,    '1',  0,    '4',  '7',  0,    0,    0,
    // 0x70 - 0x7F
    '0',  '.',  '2',  '5',  '6',  '8',  27,   0,    0,    '+',  '3',  '-',  '*',  '9',  0,    0,
    // 0x80 - 0xFF (unused, fill with 0)
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static int shift_pressed = 0;
static int release_next = 0;

// Initialize PS/2 keyboard
static void ps2_init(void) {
    *KMI_CLKDIV = 8;           // Set clock divisor
    *KMI_CR = 0x14;            // Enable KMI, enable RX
}

// Check if a key is available
static int ps2_has_key(void) {
    return (*KMI_STAT & KMI_STAT_RXFULL) ? 1 : 0;
}

// Get raw scancode (non-blocking, returns 0 if no key)
static unsigned char ps2_get_scancode(void) {
    if (*KMI_STAT & KMI_STAT_RXFULL) {
        return (unsigned char)(*KMI_DATA & 0xFF);
    }
    return 0;
}

// Get ASCII character (blocking)
static char ps2_getchar(void) {
    while (1) {
        if (*KMI_STAT & KMI_STAT_RXFULL) {
            unsigned char scancode = (unsigned char)(*KMI_DATA & 0xFF);
            
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
            
            // Convert to ASCII
            char c;
            if (shift_pressed) {
                c = scancode_set2_shift[scancode];
            } else {
                c = scancode_set2[scancode];
            }
            
            if (c != 0) {
                return c;
            }
        }
    }
}

#endif
