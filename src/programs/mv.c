/*
 * mv - Move/rename file
 * 
 * Usage: mv <source> <destination>
 * 
 * Note: Currently not fully implemented - requires filesystem write support
 */

#include "../package.h"
#include "../print.h"
#include "../functions/drivers/fat32Driver.h"

int prog_mv(const char *src, const char *dst) {
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

    // Check if destination already exists
    if (fat32_exists(dst)) {
        writeOut("Error: Destination already exists: ");
        writeOut(dst);
        writeOut("\n");
        return 1;
    }

    // TODO: Implement actual file move/rename
    // This requires:
    // 1. Finding the source directory entry
    // 2. If renaming in same directory: just update the name in entry
    // 3. If moving to different directory:
    //    - Create new entry in destination directory
    //    - Remove entry from source directory
    // 4. Writing changes back to disk
    
    writeOut("Error: mv not yet implemented (filesystem write support needed)\n");
    return 1;
}
