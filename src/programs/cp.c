/*
 * cp - Copy file
 * 
 * Usage: cp <source> <destination>
 * 
 * Note: Currently not fully implemented - requires filesystem write support
 */

#include "../package.h"
#include "../print.h"
#include "../functions/drivers/fat32Driver.h"

int prog_cp(const char *src, const char *dst) {
    if (!src || src[0] == '\0') {
        writeOut("Error: No source file specified\n");
        return 1;
    }

    if (!dst || dst[0] == '\0') {
        writeOut("Error: No destination file specified\n");
        return 1;
    }

    // Check if source exists
    if (!fat32_exists(src)) {
        writeOut("Error: Source file does not exist: ");
        writeOut(src);
        writeOut("\n");
        return 1;
    }

    // Check if source is a directory
    if (fat32_is_directory(src)) {
        writeOut("Error: Source is a directory (directory copy not supported): ");
        writeOut(src);
        writeOut("\n");
        return 1;
    }

    // Check if destination already exists
    if (fat32_exists(dst) && !fat32_is_directory(dst)) {
        writeOut("Warning: Destination already exists: ");
        writeOut(dst);
        writeOut("\n");
    }

    // TODO: Implement actual file copy
    // This requires:
    // 1. Reading the entire source file
    // 2. Finding free space for destination
    // 3. Allocating clusters in FAT
    // 4. Writing data to clusters
    // 5. Creating directory entry for destination
    // 6. Writing changes back to disk
    
    writeOut("Error: cp not yet implemented (filesystem write support needed)\n");
    return 1;
}
