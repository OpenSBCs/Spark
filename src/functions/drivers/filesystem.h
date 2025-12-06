/*
 * filesystem.h - Simple in-memory file system for Spark OS
 *
 * This is a basic RAM-based filesystem that stores files in memory.
 * Files are lost when the system reboots.
 */
#include "../../package.h"


#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#define FS_MAX_FILES     32      // Maximum number of files
#define FS_MAX_NAME      32      // Maximum filename length
#define FS_MAX_CONTENT   512     // Maximum file content size

// File types
#define FS_TYPE_FILE     1
#define FS_TYPE_DIR      2

// A single file or directory entry
typedef struct {
    char name[FS_MAX_NAME];          // Filename
    char content[FS_MAX_CONTENT];    // File contents (for files)
    int type;                        // FS_TYPE_FILE or FS_TYPE_DIR
    int size;                        // Size of content
    int used;                        // 1 if this slot is in use
} fs_entry_t;

// The filesystem - array of entries
static fs_entry_t filesystem[FS_MAX_FILES];

// Current working directory (just "/" for now - single level)
static char current_dir[FS_MAX_NAME] = "/";

// Has the filesystem been initialized?
static int fs_initialized = 0;

/*
 * fs_init - Initialize the filesystem
 * Creates some default files to demonstrate the system
 */
static void fs_init(void) {
    if (fs_initialized) return;

    // Clear all entries
    for (int i = 0; i < FS_MAX_FILES; i++) {
        filesystem[i].used = 0;
        filesystem[i].name[0] = '\0';
        filesystem[i].content[0] = '\0';
        filesystem[i].type = 0;
        filesystem[i].size = 0;
    }

    // Create some default files
    // File 0: readme.txt
    filesystem[0].used = 1;
    filesystem[0].type = FS_TYPE_FILE;
    filesystem[0].size = 45;

    // Copy filename
    const char *name0 = "readme.txt";
    for (int i = 0; name0[i] != '\0' && i < FS_MAX_NAME-1; i++) {
        filesystem[0].name[i] = name0[i];
        filesystem[0].name[i+1] = '\0';
    }

    // Copy content
    const char *content0 = "Welcome to Spark OS!\nType 'help' for commands.";
    for (int i = 0; content0[i] != '\0' && i < FS_MAX_CONTENT-1; i++) {
        filesystem[0].content[i] = content0[i];
        filesystem[0].content[i+1] = '\0';
    }

    // File 1: hello.txt
    filesystem[1].used = 1;
    filesystem[1].type = FS_TYPE_FILE;
    filesystem[1].size = 12;

    const char *name1 = "hello.txt";
    for (int i = 0; name1[i] != '\0' && i < FS_MAX_NAME-1; i++) {
        filesystem[1].name[i] = name1[i];
        filesystem[1].name[i+1] = '\0';
    }

    const char *content1 = "Hello World!";
    for (int i = 0; content1[i] != '\0' && i < FS_MAX_CONTENT-1; i++) {
        filesystem[1].content[i] = content1[i];
        filesystem[1].content[i+1] = '\0';
    }

    // File 2: notes directory
    filesystem[2].used = 1;
    filesystem[2].type = FS_TYPE_DIR;
    filesystem[2].size = 0;

    const char *name2 = "notes";
    for (int i = 0; name2[i] != '\0' && i < FS_MAX_NAME-1; i++) {
        filesystem[2].name[i] = name2[i];
        filesystem[2].name[i+1] = '\0';
    }

    fs_initialized = 1;
}

/*
 * fs_list - List all files (ls command)
 */
static void fs_list(void) {
    fs_init();

    writeOut("Directory: /\n");
    writeOut("-------------------\n");

    int count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (filesystem[i].used) {
            // Show type indicator
            if (filesystem[i].type == FS_TYPE_DIR) {
                writeOut("[DIR]  ");
            } else {
                writeOut("[FILE] ");
            }

            // Show name
            writeOut(filesystem[i].name);

            // Show size for files
            if (filesystem[i].type == FS_TYPE_FILE) {
                writeOut("  (");
                writeOutNum(filesystem[i].size);
                writeOut(" bytes)");
            }

            newline(1);
            count++;
        }
    }

    if (count == 0) {
        writeOut("(empty)\n");
    }

    newline(1);
    writeOutNum(count);
    writeOut(" items\n");
}

/*
 * fs_cat - Display file contents (cat command)
 */
static void fs_cat(const char *filename) {
    fs_init();

    // Find the file
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (filesystem[i].used && filesystem[i].type == FS_TYPE_FILE) {
            // Compare filenames
            int match = 1;
            int j = 0;
            while (filename[j] != '\0' || filesystem[i].name[j] != '\0') {
                if (filename[j] != filesystem[i].name[j]) {
                    match = 0;
                    break;
                }
                j++;
            }

            if (match) {
                writeOut(filesystem[i].content);
                newline(1);
                return;
            }
        }
    }

    writeOut("File not found: ");
    writeOut(filename);
    newline(1);
}

/*
 * fs_touch - Create an empty file
 */
static void fs_touch(const char *filename) {
    fs_init();

    // Check if file already exists
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (filesystem[i].used) {
            int match = 1;
            int j = 0;
            while (filename[j] != '\0' || filesystem[i].name[j] != '\0') {
                if (filename[j] != filesystem[i].name[j]) {
                    match = 0;
                    break;
                }
                j++;
            }
            if (match) {
                writeOut("File already exists: ");
                writeOut(filename);
                newline(1);
                return;
            }
        }
    }

    // Find empty slot
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!filesystem[i].used) {
            filesystem[i].used = 1;
            filesystem[i].type = FS_TYPE_FILE;
            filesystem[i].size = 0;
            filesystem[i].content[0] = '\0';

            // Copy filename
            int j = 0;
            while (filename[j] != '\0' && j < FS_MAX_NAME-1) {
                filesystem[i].name[j] = filename[j];
                j++;
            }
            filesystem[i].name[j] = '\0';

            writeOut("Created: ");
            writeOut(filename);
            newline(1);
            return;
        }
    }

    writeOut("Filesystem full!\n");
}

/*
 * fs_mkdir - Create a directory
 */
static void fs_mkdir(const char *dirname) {
    fs_init();

    // Check if already exists
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (filesystem[i].used) {
            int match = 1;
            int j = 0;
            while (dirname[j] != '\0' || filesystem[i].name[j] != '\0') {
                if (dirname[j] != filesystem[i].name[j]) {
                    match = 0;
                    break;
                }
                j++;
            }
            if (match) {
                writeOut("Already exists: ");
                writeOut(dirname);
                newline(1);
                return;
            }
        }
    }

    // Find empty slot
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!filesystem[i].used) {
            filesystem[i].used = 1;
            filesystem[i].type = FS_TYPE_DIR;
            filesystem[i].size = 0;

            // Copy dirname
            int j = 0;
            while (dirname[j] != '\0' && j < FS_MAX_NAME-1) {
                filesystem[i].name[j] = dirname[j];
                j++;
            }
            filesystem[i].name[j] = '\0';

            writeOut("Created directory: ");
            writeOut(dirname);
            newline(1);
            return;
        }
    }

    writeOut("Filesystem full!\n");
}

/*
 * fs_rm - Remove a file or directory
 */
static void fs_rm(const char *name) {
    fs_init();

    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (filesystem[i].used) {
            int match = 1;
            int j = 0;
            while (name[j] != '\0' || filesystem[i].name[j] != '\0') {
                if (name[j] != filesystem[i].name[j]) {
                    match = 0;
                    break;
                }
                j++;
            }

            if (match) {
                filesystem[i].used = 0;
                filesystem[i].name[0] = '\0';
                filesystem[i].content[0] = '\0';
                writeOut("Removed: ");
                writeOut(name);
                newline(1);
                return;
            }
        }
    }

    writeOut("Not found: ");
    writeOut(name);
    newline(1);
}

/*
 * fs_write - Write content to a file
 */
static void fs_write(const char *filename, const char *content) {
    fs_init();

    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (filesystem[i].used && filesystem[i].type == FS_TYPE_FILE) {
            int match = 1;
            int j = 0;
            while (filename[j] != '\0' || filesystem[i].name[j] != '\0') {
                if (filename[j] != filesystem[i].name[j]) {
                    match = 0;
                    break;
                }
                j++;
            }

            if (match) {
                // Copy content
                int k = 0;
                while (content[k] != '\0' && k < FS_MAX_CONTENT-1) {
                    filesystem[i].content[k] = content[k];
                    k++;
                }
                filesystem[i].content[k] = '\0';
                filesystem[i].size = k;

                writeOut("Wrote ");
                writeOutNum(k);
                writeOut(" bytes to ");
                writeOut(filename);
                newline(1);
                return;
            }
        }
    }

    writeOut("File not found: ");
    writeOut(filename);
    newline(1);
}

/*
 * fs_find - Find a file and return its index (-1 if not found)
 */
static int fs_find(const char *filename) {
    fs_init();

    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (filesystem[i].used) {
            int match = 1;
            int j = 0;
            while (filename[j] != '\0' || filesystem[i].name[j] != '\0') {
                if (filename[j] != filesystem[i].name[j]) {
                    match = 0;
                    break;
                }
                j++;
            }
            if (match) return i;
        }
    }
    return -1;
}

#endif
