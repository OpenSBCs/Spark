/*
 * cat - Display file contents
 *
 * Usage: cat <filename>
 */

#include <package.h>
#include <drivers/fat32Driver.h>

int prog_cat(const char *path) {
    if (!path || path[0] == '\0') {
        writeOut("Error: No file specified\n");
        return 1;
    }

    // Check if file exists
    if (!fat32_exists(path)) {
        writeOut("Error: File does not exist: ");
        writeOut(path);
        writeOut("\n");
        return 1;
    }

    // Check if it's a directory
    if (fat32_is_directory(path)) {
        writeOut("Error: Is a directory: ");
        writeOut(path);
        writeOut("\n");
        return 1;
    }

    // Read and display file contents
    static char file_buffer[4096];
    int bytes = fat32_read_file(path, file_buffer, sizeof(file_buffer) - 1);

    if (bytes >= 0) {
        file_buffer[bytes] = '\0';
        writeOut(file_buffer);
        writeOut("\n");
        return 0;
    } else {
        writeOut("Error: Could not read file\n");
        return 1;
    }
}
