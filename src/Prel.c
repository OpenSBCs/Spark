// preload script
#include "package.h"
#include "io/shell.h"
#include "functions/uart.h"
#include "functions/drivers/ps2Keyboard.h"
#include "functions/drivers/graphicsDriver.h"
// FAT32 driver — used to enumerate partitions
#include "functions/drivers/fat32Driver.h"

// Simple menu helper: display a list of items and let the user pick one
static int parse_uint(const char *s) {
    int v = 0;
    int i = 0;
    if (!s) return -1;
    while (s[i] == ' ' || s[i] == '\t') i++;
    if (s[i] == '\0') return -1;
    while (s[i]) {
        char c = s[i];
        if (c < '0' || c > '9') break;
        v = v * 10 + (c - '0');
        i++;
    }
    return v;
}

// Helper: write an ANSI cursor move sequence for n (small int)
static void write_move(char code, int n) {
    // Send CSI sequence only to UART so graphics framebuffer doesn't show raw escape bytes
    // Format: ESC [ n <code>
    uart_putchar('\x1b');
    uart_putchar('[');
    // emit n as decimal (no leading zeros)
    if (n == 0) {
        uart_putchar('0');
    } else {
        // collect digits into rev buffer
        char rev[12];
        int ri = 0;
        int tmp = n;
        while (tmp > 0 && ri < (int)sizeof(rev)) {
            rev[ri++] = '0' + (char)fat32_mod(tmp, 10);
            tmp = fat32_div(tmp, 10);
        }
        for (int k = ri - 1; k >= 0; k--) uart_putchar(rev[k]);
    }
    uart_putchar(code);
}

// Blocking get character from UART or PS/2 keyboard
static char get_input_char(void) {
    // Use the same approach as the readLine helper: check UART, then PS/2
    while (1) {
        // UART
        if (!(*UART0_FR & UART0_FR_RXFE)) {
            return (char)(*UART0_DR & 0xFF);
        }
        // PS/2 keyboard
        if (ps2_has_key()) {
            // Use ps2_getchar() from the PS/2 header (returns ASCII when available)
            char c = ps2_getchar();
            if (c != 0) return c;
        }
    }
}

static void CreateMenu(int totalItems, const char *items[]) {
    if (totalItems <= 0 || items == (void*)0) return;

    int selected = 0;
    // Print header and items once
    writeOut("Select an option (use arrow keys and press enter to lock answer):\n");
    for (int i = 0; i < totalItems; i++) {
        if (i == selected) writeOut("> "); else writeOut("  ");
        writeOutNum(i);
        writeOut(": ");
        writeOut(items[i]);
        writeOut("\n");
    }

    // (pointer update will use global write_move helper)

    while (1) {
        char c = get_input_char();

        // Enter selects current
        if (c == '\r' || c == '\n') {
            writeOut("You selected: ");
            writeOut(items[selected]);
            writeOut("\n");
            return;
        }

        int newsel = selected;
        if (c == '8' || c == 'k' || c == 'K' || c == 'w' || c == 'W') {
            newsel = (selected > 0) ? (selected - 1) : (totalItems - 1);
        } else if (c == '2' || c == 'j' || c == 'J' || c == 's' || c == 'S') {
            newsel = selected + 1;
            if (newsel >= totalItems) newsel = 0;
        } else if (c >= '0' && c <= '9') {
            // numeric entry: read rest until newline
            char buf[8];
            int idx = 0;
            buf[idx++] = c;
            while (idx < (int)(sizeof(buf)-1)) {
                char nc = get_input_char();
                if (nc == '\n' || nc == '\r') break;
                if (nc < '0' || nc > '9') break;
                buf[idx++] = nc;
            }
            buf[idx] = '\0';
            int choice = parse_uint(buf);
            if (choice >= 0 && choice < totalItems) {
                writeOut("You selected: ");
                writeOut(items[choice]);
                writeOut("\n");
                return;
            } else {
                writeOut("Invalid choice\n");
                return;
            }
        } else {
            // ignore other keys
            continue;
        }

        if (newsel != selected) {
            // clear old pointer: move up to that line, write two spaces, move back down
            int up_old = totalItems - selected;
            // Move cursor in UART then overwrite pointer column
            write_move('A', up_old);
            uart_putchar('\r'); uart_putchar(' '); uart_putchar(' ');
            write_move('B', up_old);

            // set new pointer
            int up_new = totalItems - newsel;
            write_move('A', up_new);
            uart_putchar('\r'); uart_putchar('>'); uart_putchar(' ');
            write_move('B', up_new);

            // Also update graphics framebuffer by redrawing the menu (no escape sequences)
            gfx_clear();
            gfx_print("Select an option (use arrow keys and press enter to lock answer):\n");
            for (int i = 0; i < totalItems; i++) {
                if (i == newsel) gfx_print("> "); else gfx_print("  ");
                // print index number to graphics using gfx_putchar
                int num = i;
                char numbuf[12];
                int ni = 0;
                if (num == 0) { gfx_putchar('0'); }
                else {
                    while (num > 0 && ni < (int)sizeof(numbuf)) {
                        numbuf[ni++] = '0' + (char)fat32_mod(num, 10);
                        num = fat32_div(num, 10);
                    }
                    for (int k = ni - 1; k >= 0; k--) gfx_putchar(numbuf[k]);
                }
                gfx_print(": ");
                gfx_print(items[i]);
                gfx_print("\n");
            }

            selected = newsel;
        }
    }
}

void SelectParition(void) {
    // Try to read actual partitions from MBR
    u8 types[4];
    u32 starts[4];
    u32 sizes[4];
    int count = fat32_read_partitions(types, starts, sizes, 4);

    if (count <= 0) {
        // fallback to static menu if no partitions found or read error
        const char *items[] = {
            "Partition 1 (primary)",
            "Partition 2 (primary)",
            "Partition 3 (logical)",
            "Cancel"
        };
        CreateMenu(4, items);
        return;
    }

    // Build menu strings dynamically (max 5 entries including Cancel)
    static char itembuf[5][64];
    const char *items_ptrs[5];

    for (int i = 0; i < count; i++) {
        // Format: "N: type=0xTT start=LLLL size=SSSS"
        // use writeOutNum style formatting via manual itoa into buffer
        int pos = 0;
        // index and colon already printed in CreateMenu; we include human text
        itembuf[i][0] = '\0';
        // write type
        itembuf[i][pos++] = 'T'; itembuf[i][pos++] = 'y'; itembuf[i][pos++] = 'p'; itembuf[i][pos++] = 'e'; itembuf[i][pos++] = '=';
        // hex two digits
        const char hexchars[] = "0123456789ABCDEF";
        itembuf[i][pos++] = '0'; itembuf[i][pos++] = 'x';
        itembuf[i][pos++] = hexchars[(types[i] >> 4) & 0xF];
        itembuf[i][pos++] = hexchars[types[i] & 0xF];
        itembuf[i][pos++] = ' ';
        // start=
        itembuf[i][pos++] = 's'; itembuf[i][pos++] = 't'; itembuf[i][pos++] = 'a'; itembuf[i][pos++] = 'r'; itembuf[i][pos++] = 't'; itembuf[i][pos++] = '=';
        // decimal start LBA
        u32 val = starts[i];
        char tmp[12]; int ti = 0;
        if (val == 0) { tmp[ti++] = '0'; }
        while (val > 0 && ti < (int)sizeof(tmp)) { tmp[ti++] = '0' + (char)fat32_mod(val, 10); val = fat32_div(val, 10); }
        for (int k = ti - 1; k >= 0; k--) itembuf[i][pos++] = tmp[k];
        itembuf[i][pos++] = ' ';
        // size=
        const char *size_label = "size=";
        for (int k = 0; size_label[k]; k++) itembuf[i][pos++] = size_label[k];
        val = sizes[i]; ti = 0;
        if (val == 0) { tmp[ti++] = '0'; }
        while (val > 0 && ti < (int)sizeof(tmp)) { tmp[ti++] = '0' + (char)fat32_mod(val, 10); val = fat32_div(val, 10); }
        for (int k = ti - 1; k >= 0; k--) itembuf[i][pos++] = tmp[k];
        itembuf[i][pos] = '\0';
        items_ptrs[i] = itembuf[i];
    }

    // Add Cancel entry
    int cancel_idx = count;
    items_ptrs[cancel_idx] = "Cancel";

    // Show menu and let user select
    CreateMenu(count + 1, items_ptrs);

    // After CreateMenu returns it printed the selected item; we now try to mount the selected partition
    // We don't have the selected index returned, so re-run partition read and ask user again to pick index by number.
    // Simpler: prompt the user for a numeric selection now (blocking) to get the chosen index.
    char buf[32];
    writeOut("Enter partition number to mount (or -1 to cancel): ");
    readline(buf, sizeof(buf));
    int choice = parse_uint(buf);
    if (choice < 0 || choice >= count) {
        writeOut("Mount cancelled\n");
        return;
    }

    u32 chosen_start = starts[choice];
    writeOut("Mounting partition at LBA: "); writeOutNum(chosen_start); writeOut("\n");
    int result = fat32_init(chosen_start);
    if (result == 0) {
        writeOut("FAT32 filesystem mounted successfully!\n");
    } else if (result == -1) {
        writeOut("Error: Disk read failed\n");
    } else if (result == -2) {
        writeOut("Error: Invalid boot signature\n");
    } else if (result == -3) {
        writeOut("Error: Not a FAT32 filesystem\n");
    } else {
        writeOut("Error: Mount failed\n");
    }
}
