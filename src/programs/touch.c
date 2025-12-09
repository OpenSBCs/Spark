/*
 * touch - Create empty file or update timestamp
 *
 * Usage: touch <filename>
 */

#include <package.h>
#include <drivers/writeDriver.h>

int prog_touch(const char *path) {
    if (!path || path[0] == '\0') {
        writeOut("Error: No file specified\n");
        return 1;
    }

    // Check if file already exists
    if (fat32_exists(path)) {
        writeOut("File already exists: ");
        writeOut(path);
        writeOut("\n");
        return 0;  // Success - file exists
    }

    // Create the file
    int result = fat32_create_file(path);

    if (result == 0) {
        writeOut("Created: ");
        writeOut(path);
        writeOut("\n");
        return 0;
    } else if (result == -2) {
        writeOut("Error: File already exists\n");
        return 1;
    } else if (result == -3) {
        writeOut("Error: Parent directory not found\n");
        return 1;
    } else if (result == -4) {
        writeOut("Error: Invalid filename (use 8.3 format)\n");
        return 1;
    } else if (result == -5) {
        writeOut("Error: No free directory entries\n");
        return 1;
    } else {
        writeOut("Error: Could not create file (code ");
        writeOutNum(result);
        writeOut(")\n");
        return 1;
    }
}
