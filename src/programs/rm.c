/*
 * rm - Remove file
 * 
 * Usage: rm <filename>
 * 
 * Note: Currently not fully implemented - requires filesystem write support
 */

#include "../package.h"
#include "../print.h"
#include "../functions/drivers/fat32Driver.h"

int prog_rm(const char *path) {
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
        writeOut("Error: Is a directory (use rmdir): ");
        writeOut(path);
        writeOut("\n");
        return 1;
    }

    // TODO: Implement actual file deletion
    // This requires:
    // 1. Finding the directory entry
    // 2. Marking it as deleted (0xE5)
    // 3. Freeing FAT chain clusters
    // 4. Writing changes back to disk
    
    writeOut("Error: rm not yet implemented (filesystem write support needed)\n");
    return 1;
}
