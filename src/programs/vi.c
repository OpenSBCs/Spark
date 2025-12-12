/*
 * vi - Visual Text Editor for Spark OS
 *
 * A simplified vi-like editor with support for:
 * - Normal mode: navigation and commands
 * - Insert mode: text insertion
 * - Command mode: save, quit, etc.
 *
 * Usage: vi <filename>
 */

#include <package.h>
#include <drivers/fat32Driver.h>
#include <drivers/writeDriver.h>
#include "io/print.h"
#include "io/uart.h"
#include <drivers/ps2Keyboard.h>

// ============================================================================
// Configuration
// ============================================================================

#define VI_MAX_LINES       64
#define VI_MAX_LINE_LEN    128
#define VI_MAX_FILE_SIZE   (VI_MAX_LINES * VI_MAX_LINE_LEN)
#define VI_SCREEN_ROWS     24
#define VI_SCREEN_COLS     80

// Editor modes
typedef enum {
    MODE_NORMAL,
    MODE_INSERT,
    MODE_COMMAND,
    MODE_REPLACE
} vi_mode_t;

// Editor state
typedef struct {
    char buffer[VI_MAX_LINES][VI_MAX_LINE_LEN];  // Text buffer (lines)
    int line_count;                              // Number of lines
    int cursor_row;                              // Current line (0-indexed)
    int cursor_col;                              // Current column (0-indexed)
    int scroll_offset;                           // First visible line
    vi_mode_t mode;                              // Current mode
    char filename[64];                           // Current file name
    int modified;                                // File modified flag
    char status_msg[80];                         // Status line message
    char cmd_buffer[64];                         // Command buffer for : mode
    int cmd_pos;                                 // Command buffer position
} vi_state_t;

static vi_state_t vi;

// ============================================================================
// Software Division (ARM has no hardware divider)
// ============================================================================

static unsigned int vi_div(unsigned int n, unsigned int d) {
    if (d == 0) return 0;
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

static unsigned int vi_mod(unsigned int n, unsigned int d) {
    if (d == 0) return 0;
    unsigned int r = 0;
    for (int i = 31; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) {
            r -= d;
        }
    }
    return r;
}

// ============================================================================
// Utility Functions
// ============================================================================

static int vi_strlen(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

static void vi_strcpy(char *dest, const char *src) {
    while ((*dest++ = *src++));
}

static void vi_strncpy(char *dest, const char *src, int n) {
    int i;
    for (i = 0; i < n && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

static void vi_memset(void *s, int c, int n) {
    char *p = (char *)s;
    while (n--) *p++ = c;
}

static void vi_memmove(void *dest, const void *src, int n) {
    char *d = (char *)dest;
    const char *s = (const char *)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
}

// ============================================================================
// Terminal Control (ANSI escape codes)
// ============================================================================

static void vi_clear_screen(void) {
    writeOut("\033[2J");
}

static void vi_cursor_home(void) {
    writeOut("\033[H");
}

static void vi_cursor_move(int row, int col) {
    // ANSI: ESC[row;colH (1-indexed)
    writeOut("\033[");
    writeOutNum(row + 1);
    writeOut(";");
    writeOutNum(col + 1);
    writeOut("H");
}

static void vi_clear_line(void) {
    writeOut("\033[K");
}

static void vi_inverse_on(void) {
    writeOut("\033[7m");
}

static void vi_inverse_off(void) {
    writeOut("\033[0m");
}

static void vi_hide_cursor(void) {
    writeOut("\033[?25l");
}

static void vi_show_cursor(void) {
    writeOut("\033[?25h");
}

// ============================================================================
// Input Handling
// ============================================================================

// Special key codes (returned as negative values to distinguish from ASCII)
#define KEY_UP      -1
#define KEY_DOWN    -2
#define KEY_LEFT    -3
#define KEY_RIGHT   -4

// Check if UART has data
static int vi_uart_has_data(void) {
    return !(*UART0_FR & UART0_FR_RXFE);
}

// Extended key state for PS/2
static int extended_key = 0;

// Get a character from either UART or PS/2 keyboard
// Returns ASCII char, or negative KEY_* values for special keys
static int vi_getchar(void) {
    ps2_init();
    while (1) {
        // Check UART first
        if (vi_uart_has_data()) {
            return (int)(*UART0_DR & 0xFF);
        }
        // Check PS/2 keyboard
        if (ps2_has_key()) {
            unsigned char scancode = ps2_get_scancode();

            // 0xE0 = extended key prefix (arrow keys, etc.)
            if (scancode == 0xE0) {
                extended_key = 1;
                continue;
            }

            // 0xF0 = key release prefix
            if (scancode == 0xF0) {
                release_next = 1;
                continue;
            }

            if (release_next) {
                release_next = 0;
                extended_key = 0;
                if (scancode == 0x12 || scancode == 0x59) {
                    shift_pressed = 0;
                }
                if (scancode == 0x14) {
                    ctrl_pressed = 0;
                }
                continue;
            }

            // Handle extended keys (arrow keys with E0 prefix)
            if (extended_key) {
                extended_key = 0;
                switch (scancode) {
                    case 0x75: return KEY_UP;
                    case 0x72: return KEY_DOWN;
                    case 0x6B: return KEY_LEFT;
                    case 0x74: return KEY_RIGHT;
                }
                continue;  // Unknown extended key, ignore
            }

            // Also handle arrow keys WITHOUT E0 prefix (QEMU sometimes does this)
            // These are the same scancodes as numpad keys but we prioritize arrow behavior
            switch (scancode) {
                case 0x75: return KEY_UP;     // Up arrow / numpad 8
                case 0x72: return KEY_DOWN;   // Down arrow / numpad 2
                case 0x6B: return KEY_LEFT;   // Left arrow / numpad 4
                case 0x74: return KEY_RIGHT;  // Right arrow / numpad 6
            }

            // Shift keys
            if (scancode == 0x12 || scancode == 0x59) {
                shift_pressed = 1;
                continue;
            }

            // Ctrl key (0x14)
            if (scancode == 0x14) {
                ctrl_pressed = 1;
                continue;
            }

            char c = shift_pressed ?
                scancode_set2_shift[scancode] :
                scancode_set2[scancode];

            if (c != 0) return (int)c;
        }
    }
}

// ============================================================================
// File Operations
// ============================================================================

static int vi_load_file(const char *path) {
    static char file_buf[VI_MAX_FILE_SIZE];
    
    // Initialize buffer
    vi.line_count = 1;
    vi_memset(vi.buffer, 0, sizeof(vi.buffer));
    
    if (!fat32_exists(path)) {
        // New file
        vi_strcpy(vi.status_msg, "[New File]");
        return 0;
    }
    
    if (fat32_is_directory(path)) {
        vi_strcpy(vi.status_msg, "Error: Is a directory");
        return -1;
    }
    
    int bytes = fat32_read_file(path, file_buf, sizeof(file_buf) - 1);
    if (bytes < 0) {
        vi_strcpy(vi.status_msg, "Error reading file");
        return -1;
    }
    
    file_buf[bytes] = '\0';
    
    // Parse file into lines
    int line = 0;
    int col = 0;
    for (int i = 0; i < bytes && line < VI_MAX_LINES; i++) {
        if (file_buf[i] == '\n') {
            vi.buffer[line][col] = '\0';
            line++;
            col = 0;
        } else if (file_buf[i] != '\r') {
            if (col < VI_MAX_LINE_LEN - 1) {
                vi.buffer[line][col++] = file_buf[i];
            }
        }
    }
    
    // Null-terminate last line
    vi.buffer[line][col] = '\0';
    vi.line_count = line + 1;
    
    // Status message
    vi_strcpy(vi.status_msg, "\"");
    int slen = vi_strlen(vi.status_msg);
    vi_strncpy(vi.status_msg + slen, path, 40);
    slen = vi_strlen(vi.status_msg);
    vi_strcpy(vi.status_msg + slen, "\" ");
    slen = vi_strlen(vi.status_msg);
    
    return 0;
}

static int vi_save_file(void) {
    static char file_buf[VI_MAX_FILE_SIZE];
    int pos = 0;
    
    // Build file content from lines
    for (int i = 0; i < vi.line_count; i++) {
        int len = vi_strlen(vi.buffer[i]);
        for (int j = 0; j < len && pos < VI_MAX_FILE_SIZE - 2; j++) {
            file_buf[pos++] = vi.buffer[i][j];
        }
        if (i < vi.line_count - 1 && pos < VI_MAX_FILE_SIZE - 1) {
            file_buf[pos++] = '\n';
        }
    }
    file_buf[pos] = '\0';
    
    int result = fat32_write_file(vi.filename, file_buf, pos);
    
    if (result >= 0) {
        vi.modified = 0;
        vi_strcpy(vi.status_msg, "Written ");
        int slen = vi_strlen(vi.status_msg);
        // Count bytes written
        int digits[12];
        int num_digits = 0;
        int n = pos;
        if (n == 0) {
            digits[num_digits++] = 0;
        }
        while (n > 0) {
            digits[num_digits++] = vi_mod(n, 10);
            n = vi_div(n, 10);
        }
        for (int i = num_digits - 1; i >= 0; i--) {
            vi.status_msg[slen++] = '0' + digits[i];
        }
        vi_strcpy(vi.status_msg + slen, " bytes");
        return 0;
    } else {
        // Show specific error code
        vi_strcpy(vi.status_msg, "Write error: ");
        int slen = vi_strlen(vi.status_msg);
        int err = -result;  // Make positive
        if (err == 1) vi_strcpy(vi.status_msg + slen, "FS not init");
        else if (err == 2) vi_strcpy(vi.status_msg + slen, "Is directory");
        else if (err == 3) vi_strcpy(vi.status_msg + slen, "Bad parent");
        else if (err == 4) vi_strcpy(vi.status_msg + slen, "No space");
        else if (err == 5) vi_strcpy(vi.status_msg + slen, "Write failed");
        else {
            vi.status_msg[slen++] = '0' + err;
            vi.status_msg[slen] = '\0';
        }
        return -1;
    }
}

// ============================================================================
// Display
// ============================================================================

// Cursor blink state (toggled each screen draw)
static int cursor_visible = 1;

static void vi_draw_screen(void) {
    vi_hide_cursor();
    vi_cursor_home();
    
    // Toggle cursor blink
    cursor_visible = !cursor_visible;
    
    // Calculate screen position of cursor
    int cursor_screen_row = vi.cursor_row - vi.scroll_offset;
    
    // Draw text lines
    for (int i = 0; i < VI_SCREEN_ROWS - 1; i++) {
        int line_num = vi.scroll_offset + i;
        
        vi_cursor_move(i, 0);
        vi_clear_line();
        
        if (line_num < vi.line_count) {
            // Draw the line content (truncate if too long)
            int len = vi_strlen(vi.buffer[line_num]);
            for (int j = 0; j < len && j < VI_SCREEN_COLS - 1; j++) {
                // Check if this is cursor position - draw inverse or underscore
                if (i == cursor_screen_row && j == vi.cursor_col) {
                    vi_inverse_on();
                    char ch[2] = {vi.buffer[line_num][j], '\0'};
                    writeOut(ch);
                    vi_inverse_off();
                } else {
                    char ch[2] = {vi.buffer[line_num][j], '\0'};
                    writeOut(ch);
                }
            }
            // If cursor is at end of line (insert mode), draw block cursor
            if (i == cursor_screen_row && vi.cursor_col >= len) {
                vi_inverse_on();
                writeOut(" ");
                vi_inverse_off();
            }
        } else {
            // Draw tilde for empty lines
            writeOut("~");
        }
    }
    
    // Draw status line
    vi_cursor_move(VI_SCREEN_ROWS - 1, 0);
    vi_inverse_on();
    
    // Left side: filename and modified flag
    writeOut(" ");
    if (vi.filename[0]) {
        writeOut(vi.filename);
    } else {
        writeOut("[No Name]");
    }
    if (vi.modified) {
        writeOut(" [+]");
    }
    
    // Mode indicator
    writeOut(" - ");
    switch (vi.mode) {
        case MODE_NORMAL:  writeOut("NORMAL"); break;
        case MODE_INSERT:  writeOut("-- INSERT --"); break;
        case MODE_REPLACE: writeOut("-- REPLACE --"); break;
        case MODE_COMMAND: writeOut(":"); writeOut(vi.cmd_buffer); break;
    }
    
    // Right side: position
    writeOut(" | Line ");
    writeOutNum(vi.cursor_row + 1);
    writeOut("/");
    writeOutNum(vi.line_count);
    writeOut(", Col ");
    writeOutNum(vi.cursor_col + 1);
    writeOut(" ");
    
    // Clear rest of status line
    vi_clear_line();
    vi_inverse_off();
    
    // Show status message if any
    if (vi.status_msg[0] && vi.mode != MODE_COMMAND) {
        vi_cursor_move(VI_SCREEN_ROWS - 1, 50);
        writeOut(vi.status_msg);
    }
    
    // Position cursor
    vi_cursor_move(vi.cursor_row - vi.scroll_offset, vi.cursor_col);
    vi_show_cursor();
}

// Ensure cursor is visible on screen
static void vi_scroll_to_cursor(void) {
    if (vi.cursor_row < vi.scroll_offset) {
        vi.scroll_offset = vi.cursor_row;
    }
    if (vi.cursor_row >= vi.scroll_offset + VI_SCREEN_ROWS - 1) {
        vi.scroll_offset = vi.cursor_row - VI_SCREEN_ROWS + 2;
    }
}

// Clamp cursor column to valid range
static void vi_clamp_cursor(void) {
    int line_len = vi_strlen(vi.buffer[vi.cursor_row]);
    if (vi.mode == MODE_INSERT) {
        // In insert mode, cursor can be at end of line
        if (vi.cursor_col > line_len) {
            vi.cursor_col = line_len;
        }
    } else {
        // In normal mode, cursor must be on a character
        if (line_len == 0) {
            vi.cursor_col = 0;
        } else if (vi.cursor_col >= line_len) {
            vi.cursor_col = line_len - 1;
        }
    }
    if (vi.cursor_col < 0) vi.cursor_col = 0;
}

// ============================================================================
// Text Editing Operations
// ============================================================================

static void vi_insert_char(char c) {
    int line_len = vi_strlen(vi.buffer[vi.cursor_row]);
    
    if (line_len >= VI_MAX_LINE_LEN - 1) return;
    
    // Shift characters right
    vi_memmove(&vi.buffer[vi.cursor_row][vi.cursor_col + 1],
               &vi.buffer[vi.cursor_row][vi.cursor_col],
               line_len - vi.cursor_col + 1);
    
    vi.buffer[vi.cursor_row][vi.cursor_col] = c;
    vi.cursor_col++;
    vi.modified = 1;
}

static void vi_insert_newline(void) {
    if (vi.line_count >= VI_MAX_LINES) return;
    
    // Make room for new line
    for (int i = vi.line_count; i > vi.cursor_row + 1; i--) {
        vi_strcpy(vi.buffer[i], vi.buffer[i - 1]);
    }
    
    // Split current line
    vi_strcpy(vi.buffer[vi.cursor_row + 1], &vi.buffer[vi.cursor_row][vi.cursor_col]);
    vi.buffer[vi.cursor_row][vi.cursor_col] = '\0';
    
    vi.line_count++;
    vi.cursor_row++;
    vi.cursor_col = 0;
    vi.modified = 1;
}

static void vi_delete_char(void) {
    int line_len = vi_strlen(vi.buffer[vi.cursor_row]);
    
    if (vi.cursor_col >= line_len) return;
    
    // Shift characters left
    vi_memmove(&vi.buffer[vi.cursor_row][vi.cursor_col],
               &vi.buffer[vi.cursor_row][vi.cursor_col + 1],
               line_len - vi.cursor_col);
    
    vi.modified = 1;
}

static void vi_backspace(void) {
    if (vi.cursor_col > 0) {
        vi.cursor_col--;
        vi_delete_char();
    } else if (vi.cursor_row > 0) {
        // Join with previous line
        int prev_len = vi_strlen(vi.buffer[vi.cursor_row - 1]);
        int curr_len = vi_strlen(vi.buffer[vi.cursor_row]);
        
        if (prev_len + curr_len < VI_MAX_LINE_LEN - 1) {
            // Append current line to previous
            vi_strcpy(&vi.buffer[vi.cursor_row - 1][prev_len],
                     vi.buffer[vi.cursor_row]);
            
            // Shift lines up
            for (int i = vi.cursor_row; i < vi.line_count - 1; i++) {
                vi_strcpy(vi.buffer[i], vi.buffer[i + 1]);
            }
            
            vi.line_count--;
            vi.cursor_row--;
            vi.cursor_col = prev_len;
            vi.modified = 1;
        }
    }
}

static void vi_delete_line(void) {
    if (vi.line_count == 1) {
        // Clear the only line
        vi.buffer[0][0] = '\0';
        vi.cursor_col = 0;
    } else {
        // Shift lines up
        for (int i = vi.cursor_row; i < vi.line_count - 1; i++) {
            vi_strcpy(vi.buffer[i], vi.buffer[i + 1]);
        }
        vi.line_count--;
        
        if (vi.cursor_row >= vi.line_count) {
            vi.cursor_row = vi.line_count - 1;
        }
    }
    vi.cursor_col = 0;
    vi.modified = 1;
}

static void vi_join_lines(void) {
    if (vi.cursor_row >= vi.line_count - 1) return;
    
    int curr_len = vi_strlen(vi.buffer[vi.cursor_row]);
    int next_len = vi_strlen(vi.buffer[vi.cursor_row + 1]);
    
    if (curr_len + next_len + 1 < VI_MAX_LINE_LEN) {
        // Add space between lines
        if (curr_len > 0) {
            vi.buffer[vi.cursor_row][curr_len++] = ' ';
        }
        vi_strcpy(&vi.buffer[vi.cursor_row][curr_len],
                 vi.buffer[vi.cursor_row + 1]);
        
        // Shift lines up
        for (int i = vi.cursor_row + 1; i < vi.line_count - 1; i++) {
            vi_strcpy(vi.buffer[i], vi.buffer[i + 1]);
        }
        vi.line_count--;
        vi.modified = 1;
    }
}

static void vi_open_line_below(void) {
    if (vi.line_count >= VI_MAX_LINES) return;
    
    // Make room for new line
    for (int i = vi.line_count; i > vi.cursor_row + 1; i--) {
        vi_strcpy(vi.buffer[i], vi.buffer[i - 1]);
    }
    
    vi.buffer[vi.cursor_row + 1][0] = '\0';
    vi.line_count++;
    vi.cursor_row++;
    vi.cursor_col = 0;
    vi.mode = MODE_INSERT;
    vi.modified = 1;
}

static void vi_open_line_above(void) {
    if (vi.line_count >= VI_MAX_LINES) return;
    
    // Make room for new line
    for (int i = vi.line_count; i > vi.cursor_row; i--) {
        vi_strcpy(vi.buffer[i], vi.buffer[i - 1]);
    }
    
    vi.buffer[vi.cursor_row][0] = '\0';
    vi.line_count++;
    vi.cursor_col = 0;
    vi.mode = MODE_INSERT;
    vi.modified = 1;
}

// ============================================================================
// Command Mode
// ============================================================================

static int vi_execute_command(void) {
    // Clear previous message
    vi.status_msg[0] = '\0';
    
    if (strcmp(vi.cmd_buffer, "q") == 0) {
        if (vi.modified) {
            vi_strcpy(vi.status_msg, "No write since last change (add ! to override)");
            return 0;
        }
        return 1;  // Quit
    }
    
    if (strcmp(vi.cmd_buffer, "q!") == 0) {
        return 1;  // Force quit
    }
    
    if (strcmp(vi.cmd_buffer, "w") == 0) {
        if (!vi.filename[0]) {
            vi_strcpy(vi.status_msg, "No file name");
            return 0;
        }
        vi_save_file();
        return 0;
    }
    
    if (strcmp(vi.cmd_buffer, "wq") == 0 || strcmp(vi.cmd_buffer, "x") == 0) {
        if (!vi.filename[0]) {
            vi_strcpy(vi.status_msg, "No file name");
            return 0;
        }
        if (vi_save_file() == 0) {
            return 1;  // Quit after save
        }
        return 0;
    }
    
    // :w filename - save as
    if (vi.cmd_buffer[0] == 'w' && vi.cmd_buffer[1] == ' ') {
        vi_strncpy(vi.filename, &vi.cmd_buffer[2], sizeof(vi.filename) - 1);
        vi.filename[sizeof(vi.filename) - 1] = '\0';
        vi_save_file();
        return 0;
    }
    
    // Line number
    int is_num = 1;
    for (int i = 0; vi.cmd_buffer[i]; i++) {
        if (vi.cmd_buffer[i] < '0' || vi.cmd_buffer[i] > '9') {
            is_num = 0;
            break;
        }
    }
    if (is_num && vi.cmd_buffer[0]) {
        int line = 0;
        for (int i = 0; vi.cmd_buffer[i]; i++) {
            line = line * 10 + (vi.cmd_buffer[i] - '0');
        }
        if (line > 0 && line <= vi.line_count) {
            vi.cursor_row = line - 1;
            vi.cursor_col = 0;
            vi_clamp_cursor();
        }
        return 0;
    }
    
    if (vi.cmd_buffer[0]) {
        vi_strcpy(vi.status_msg, "Unknown command: ");
        int slen = vi_strlen(vi.status_msg);
        vi_strncpy(vi.status_msg + slen, vi.cmd_buffer, 30);
    }
    
    return 0;
}

// ============================================================================
// Mode Handlers
// ============================================================================

static int vi_handle_normal(int c) {
    int line_len;
    
    vi.status_msg[0] = '\0';  // Clear status message
    
    // Handle arrow keys first
    if (c == KEY_UP) {
        if (vi.cursor_row > 0) vi.cursor_row--;
        vi_clamp_cursor();
        return 0;
    }
    if (c == KEY_DOWN) {
        if (vi.cursor_row < vi.line_count - 1) vi.cursor_row++;
        vi_clamp_cursor();
        return 0;
    }
    if (c == KEY_LEFT) {
        if (vi.cursor_col > 0) vi.cursor_col--;
        return 0;
    }
    if (c == KEY_RIGHT) {
        line_len = vi_strlen(vi.buffer[vi.cursor_row]);
        if (vi.cursor_col < line_len - 1) vi.cursor_col++;
        return 0;
    }
    
    switch (c) {
        // Movement
        case 'h':  // Left
            if (vi.cursor_col > 0) vi.cursor_col--;
            break;
        case 'j':  // Down
            if (vi.cursor_row < vi.line_count - 1) vi.cursor_row++;
            vi_clamp_cursor();
            break;
        case 'k':  // Up
            if (vi.cursor_row > 0) vi.cursor_row--;
            vi_clamp_cursor();
            break;
        case 'l':  // Right
            line_len = vi_strlen(vi.buffer[vi.cursor_row]);
            if (vi.cursor_col < line_len - 1) vi.cursor_col++;
            break;
            
        // Word movement
        case 'w':  // Word forward
            line_len = vi_strlen(vi.buffer[vi.cursor_row]);
            // Skip current word
            while (vi.cursor_col < line_len && 
                   vi.buffer[vi.cursor_row][vi.cursor_col] != ' ') {
                vi.cursor_col++;
            }
            // Skip spaces
            while (vi.cursor_col < line_len && 
                   vi.buffer[vi.cursor_row][vi.cursor_col] == ' ') {
                vi.cursor_col++;
            }
            vi_clamp_cursor();
            break;
        case 'b':  // Word backward
            if (vi.cursor_col > 0) vi.cursor_col--;
            // Skip spaces
            while (vi.cursor_col > 0 && 
                   vi.buffer[vi.cursor_row][vi.cursor_col] == ' ') {
                vi.cursor_col--;
            }
            // Skip word
            while (vi.cursor_col > 0 && 
                   vi.buffer[vi.cursor_row][vi.cursor_col - 1] != ' ') {
                vi.cursor_col--;
            }
            break;
            
        case '0':  // Beginning of line
            vi.cursor_col = 0;
            break;
        case '$':  // End of line
            line_len = vi_strlen(vi.buffer[vi.cursor_row]);
            vi.cursor_col = line_len > 0 ? line_len - 1 : 0;
            break;
        case '^':  // First non-space
            vi.cursor_col = 0;
            while (vi.buffer[vi.cursor_row][vi.cursor_col] == ' ') {
                vi.cursor_col++;
            }
            vi_clamp_cursor();
            break;
            
        case 'g':  // gg = go to first line (simplified)
            vi.cursor_row = 0;
            vi.cursor_col = 0;
            break;
        case 'G':  // Go to last line
            vi.cursor_row = vi.line_count - 1;
            vi.cursor_col = 0;
            vi_clamp_cursor();
            break;
            
        // Insert modes
        case 'i':  // Insert before cursor
            vi.mode = MODE_INSERT;
            break;
        case 'a':  // Insert after cursor
            line_len = vi_strlen(vi.buffer[vi.cursor_row]);
            if (line_len > 0) vi.cursor_col++;
            vi.mode = MODE_INSERT;
            break;
        case 'I':  // Insert at beginning of line
            vi.cursor_col = 0;
            vi.mode = MODE_INSERT;
            break;
        case 'A':  // Insert at end of line
            vi.cursor_col = vi_strlen(vi.buffer[vi.cursor_row]);
            vi.mode = MODE_INSERT;
            break;
        case 'o':  // Open line below
            vi_open_line_below();
            break;
        case 'O':  // Open line above
            vi_open_line_above();
            break;
        case 'R':  // Replace mode
            vi.mode = MODE_REPLACE;
            break;
            
        // Editing
        case 'x':  // Delete character
            vi_delete_char();
            vi_clamp_cursor();
            break;
        case 'X':  // Delete character before cursor
            if (vi.cursor_col > 0) {
                vi.cursor_col--;
                vi_delete_char();
            }
            break;
        case 'd':  // Delete line (simplified: dd)
            vi_delete_line();
            break;
        case 'D':  // Delete to end of line
            vi.buffer[vi.cursor_row][vi.cursor_col] = '\0';
            vi_clamp_cursor();
            vi.modified = 1;
            break;
        case 'J':  // Join lines
            vi_join_lines();
            break;
        case 'r':  // Replace single character
            {
                char replacement = vi_getchar();
                if (replacement >= 32 && replacement < 127) {
                    line_len = vi_strlen(vi.buffer[vi.cursor_row]);
                    if (vi.cursor_col < line_len) {
                        vi.buffer[vi.cursor_row][vi.cursor_col] = replacement;
                        vi.modified = 1;
                    }
                }
            }
            break;
            
        // Command mode
        case ':':
            vi.mode = MODE_COMMAND;
            vi.cmd_buffer[0] = '\0';
            vi.cmd_pos = 0;
            break;
            
        // Escape (already in normal mode)
        case 27:
            break;
            
        default:
            break;
    }
    
    return 0;
}

static int vi_handle_insert(int c) {
    // Handle arrow keys in insert mode
    if (c == KEY_UP) {
        if (vi.cursor_row > 0) vi.cursor_row--;
        vi_clamp_cursor();
        return 0;
    }
    if (c == KEY_DOWN) {
        if (vi.cursor_row < vi.line_count - 1) vi.cursor_row++;
        vi_clamp_cursor();
        return 0;
    }
    if (c == KEY_LEFT) {
        if (vi.cursor_col > 0) vi.cursor_col--;
        return 0;
    }
    if (c == KEY_RIGHT) {
        int line_len = vi_strlen(vi.buffer[vi.cursor_row]);
        if (vi.cursor_col < line_len) vi.cursor_col++;
        return 0;
    }
    
    switch (c) {
        case 27:  // Escape - return to normal mode
            vi.mode = MODE_NORMAL;
            if (vi.cursor_col > 0) vi.cursor_col--;
            vi_clamp_cursor();
            break;
            
        case '\n':
        case '\r':
            vi_insert_newline();
            break;
            
        case '\b':
        case 127:  // Backspace/Delete
            vi_backspace();
            break;
            
        case '\t':  // Tab - insert 4 spaces
            for (int i = 0; i < 4; i++) vi_insert_char(' ');
            break;
            
        default:
            if (c >= 32 && c < 127) {
                vi_insert_char((char)c);
            }
            break;
    }
    
    return 0;
}

static int vi_handle_replace(int c) {
    int line_len;
    
    // Handle arrow keys
    if (c == KEY_UP || c == KEY_DOWN || c == KEY_LEFT || c == KEY_RIGHT) {
        return vi_handle_insert(c);  // Reuse insert mode arrow handling
    }
    
    switch (c) {
        case 27:  // Escape - return to normal mode
            vi.mode = MODE_NORMAL;
            if (vi.cursor_col > 0) vi.cursor_col--;
            vi_clamp_cursor();
            break;
            
        case '\n':
        case '\r':
            vi_insert_newline();
            break;
            
        case '\b':
        case 127:  // Backspace
            if (vi.cursor_col > 0) vi.cursor_col--;
            break;
            
        default:
            if (c >= 32 && c < 127) {
                line_len = vi_strlen(vi.buffer[vi.cursor_row]);
                if (vi.cursor_col < line_len) {
                    vi.buffer[vi.cursor_row][vi.cursor_col] = (char)c;
                    vi.cursor_col++;
                    vi.modified = 1;
                } else {
                    vi_insert_char((char)c);
                }
            }
            break;
    }
    
    return 0;
}

static int vi_handle_command(int c) {
    // Ignore arrow keys in command mode
    if (c < 0) return 0;
    
    switch (c) {
        case 27:  // Escape - cancel command
            vi.mode = MODE_NORMAL;
            vi.cmd_buffer[0] = '\0';
            vi.cmd_pos = 0;
            break;
            
        case '\n':
        case '\r':
            vi.mode = MODE_NORMAL;
            if (vi_execute_command()) {
                return 1;  // Exit signal
            }
            vi.cmd_buffer[0] = '\0';
            vi.cmd_pos = 0;
            break;
            
        case '\b':
        case 127:  // Backspace
            if (vi.cmd_pos > 0) {
                vi.cmd_pos--;
                vi.cmd_buffer[vi.cmd_pos] = '\0';
            } else {
                vi.mode = MODE_NORMAL;
            }
            break;
            
        default:
            if (c >= 32 && c < 127 && vi.cmd_pos < 62) {
                vi.cmd_buffer[vi.cmd_pos++] = (char)c;
                vi.cmd_buffer[vi.cmd_pos] = '\0';
            }
            break;
    }
    
    return 0;
}

// ============================================================================
// Main Entry Point
// ============================================================================

void prog_vi(const char *filename) {
    // Check filesystem
    if (!fat32_is_initialized()) {
        writeOut("Error: Filesystem not mounted. Run 'setup' first.\n");
        return;
    }
    
    // Initialize editor state
    vi_memset(&vi, 0, sizeof(vi));
    vi.line_count = 1;
    vi.mode = MODE_NORMAL;
    
    // Copy filename
    if (filename && filename[0]) {
        vi_strncpy(vi.filename, filename, sizeof(vi.filename) - 1);
        vi.filename[sizeof(vi.filename) - 1] = '\0';
    }
    
    // Load file if exists
    if (vi.filename[0]) {
        vi_load_file(vi.filename);
    } else {
        vi_strcpy(vi.status_msg, "[New File]");
    }
    
    // Clear screen and start
    vi_clear_screen();
    
    // Main loop
    int running = 1;
    while (running) {
        vi_scroll_to_cursor();
        vi_draw_screen();
        
        int c = vi_getchar();
        
        switch (vi.mode) {
            case MODE_NORMAL:
                running = !vi_handle_normal(c);
                break;
            case MODE_INSERT:
                running = !vi_handle_insert(c);
                break;
            case MODE_REPLACE:
                running = !vi_handle_replace(c);
                break;
            case MODE_COMMAND:
                running = !vi_handle_command(c);
                break;
        }
    }
    
    // Minimal exit - just return to shell
    // Don't do any cleanup that might crash
}
