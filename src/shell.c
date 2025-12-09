#include "package.h"
#include "print.h"
#include "functions/drivers/fat32Driver.h"

void setup_process(void);  // Forward declaration

// External program declarations
extern int prog_rm(const char *path);
extern int prog_cat(const char *path);
extern int prog_cp(const char *src, const char *dst);
extern int prog_mv(const char *src, const char *dst);
extern int prog_touch(const char *path);

// Current working directory (simple implementation)
static char current_dir[256] = "/";

// Helper to extract argument from command
static const char* get_arg(const char *cmd, const char *prefix) {
    int prefix_len = 0;
    while (prefix[prefix_len]) prefix_len++;
    
    // Skip prefix and space
    const char *arg = cmd + prefix_len;
    while (*arg == ' ') arg++;
    
    return (*arg) ? arg : (void*)0;
}

int sh_exec(const char *cmd) {
    if (strcmp(cmd, "help") == 0) {
        print(
            "COMMANDS\n"
            "  SYSTEM\n"
            "    help          Show this help menu\n"
            "    about         Show info about Spark\n"
            "    exit          Shutdown Spark\n"
            "    setup/ssw     Run setup wizard (use 'part' to mount disk)\n"
            "\n"
            "  FILES\n"
            "    ls [path]     List directory contents\n"
            "    cat <file>    Display file contents (external)\n"
            "    rm <file>     Remove file (external)\n"
            "    cp <s> <d>    Copy file (external)\n"
            "    mv <s> <d>    Move/rename file (external)\n"
            "    touch <file>  Create empty file (external)\n"
            "    exists <path> Check if file/dir exists\n"
            "\n"
            "  DIRECTORY\n"
            "    cd [path]     Change directory\n"
            "    pwd           Print working directory\n"
            "    mkdir <dir>   Create directory\n"
            "\n"
            "  UTILITIES\n"
            "    echo [text]   Print text to screen\n"
            "    clear         Clear screen\n"
            "\n"
        );
    }
    else if (strcmp(cmd, "about") == 0) {
        print(
            "Spark is developed by syntaxMORG0 and Samuraien2\n"
            "You can find the Spark project at https://github.com/OpenSBCs/Spark\n"
        );
    }
    else if (strcmp(cmd, "exit") == 0) {
        return 66;
    }
    else if (strcmp(cmd, "setup") == 0 || strcmp(cmd, "ssw") == 0) {
        setup_process();
    }
    else if (strcmp(cmd, "ls") == 0) {
        if (!fat32_is_initialized()) {
            writeOut("Error: Filesystem not mounted. Run 'setup' then 'part'.\n");
        } else {
            fat32_list_dir("/");
        }
    }
    else if (startsWith(cmd, "ls ")) {
        if (!fat32_is_initialized()) {
            writeOut("Error: Filesystem not mounted. Run 'setup' then 'part'.\n");
        } else {
            const char *path = get_arg(cmd, "ls");
            if (path) {
                fat32_list_dir(path);
            } else {
                fat32_list_dir("/");
            }
        }
    }
    else if (startsWith(cmd, "exists ")) {
        if (!fat32_is_initialized()) {
            writeOut("Error: Filesystem not mounted. Run 'setup' then 'part'.\n");
        } else {
            const char *path = get_arg(cmd, "exists");
            if (path) {
                if (fat32_exists(path)) {
                    writeOut("Path exists");
                    if (fat32_is_directory(path)) {
                        writeOut(" (directory)");
                    } else {
                        writeOut(" (file)");
                    }
                    writeOut("\n");
                } else {
                    writeOut("Path does not exist\n");
                }
            } else {
                writeOut("Usage: exists <path>\n");
            }
        }
    }
    // Builtin: pwd
    else if (strcmp(cmd, "pwd") == 0) {
        print(current_dir, "\n");
    }
    // Builtin: cd
    else if (strcmp(cmd, "cd") == 0) {
        // cd with no args goes to root
        strcpy(current_dir, "/");
    }
    else if (startsWith(cmd, "cd ")) {
        const char *path = get_arg(cmd, "cd");
        if (path) {
            if (!fat32_is_initialized()) {
                writeOut("Error: Filesystem not mounted. Run 'setup' then 'part'.\n");
            } else {
                if (fat32_exists(path)) {
                    if (fat32_is_directory(path)) {
                        int i = 0;
                        while (path[i] && i < 255) {
                            current_dir[i] = path[i];
                            i++;
                        }
                        current_dir[i] = '\0';
                    } else {
                        writeOut("Error: Not a directory\n");
                    }
                } else {
                    writeOut("Error: Directory does not exist\n");
                }
            }
        }
    }
    // Builtin: clear
    else if (strcmp(cmd, "clear") == 0) {
        writeOut("\033[2J\033[H");  // ANSI escape codes: clear screen and move cursor to home
    }
    // Builtin: mkdir
    else if (startsWith(cmd, "mkdir ")) {
        writeOut("Error: mkdir not implemented (filesystem write support needed)\n");
    }   
    // External program: cat
    else if (startsWith(cmd, "cat ")) {
        const char *path = get_arg(cmd, "cat");
        if (path) {
            if (!fat32_is_initialized()) {
                writeOut("Error: Filesystem not mounted. Run 'setup' then 'part'.\n");
            } else {
                return prog_cat(path);
            }
        } else {
            writeOut("Usage: cat <filename>\n");
        }
    }
    // External program: rm
    else if (startsWith(cmd, "rm ")) {
        const char *path = get_arg(cmd, "rm");
        if (path) {
            if (!fat32_is_initialized()) {
                writeOut("Error: Filesystem not mounted. Run 'setup' then 'part'.\n");
            } else {
                return prog_rm(path);
            }
        } else {
            writeOut("Usage: rm <filename>\n");
        }
    }
    // External program: touch
    else if (startsWith(cmd, "touch ")) {
        const char *path = get_arg(cmd, "touch");
        if (path) {
            if (!fat32_is_initialized()) {
                writeOut("Error: Filesystem not mounted. Run 'setup' then 'part'.\n");
            } else {
                return prog_touch(path);
            }
        } else {
            writeOut("Usage: touch <filename>\n");
        }
    }
    // External program: cp
    else if (startsWith(cmd, "cp ")) {
        // Parse two arguments
        const char *args = get_arg(cmd, "cp");
        if (args) {
            // Find first space to split arguments
            const char *space = args;
            while (*space && *space != ' ') space++;
            
            if (*space) {
                // Create src string
                static char src[256];
                int i = 0;
                while (args < space && i < 255) {
                    src[i++] = *args++;
                }
                src[i] = '\0';
                
                // Skip spaces
                while (*space == ' ') space++;
                
                if (*space && !fat32_is_initialized()) {
                    writeOut("Error: Filesystem not mounted. Run 'setup' then 'part'.\n");
                } else if (*space) {
                    return prog_cp(src, space);
                } else {
                    writeOut("Usage: cp <source> <destination>\n");
                }
            } else {
                writeOut("Usage: cp <source> <destination>\n");
            }
        } else {
            writeOut("Usage: cp <source> <destination>\n");
        }
    }
    // External program: mv
    else if (startsWith(cmd, "mv ")) {
        // Parse two arguments
        const char *args = get_arg(cmd, "mv");
        if (args) {
            // Find first space to split arguments
            const char *space = args;
            while (*space && *space != ' ') space++;
            
            if (*space) {
                // Create src string
                static char src[256];
                int i = 0;
                while (args < space && i < 255) {
                    src[i++] = *args++;
                }
                src[i] = '\0';
                
                // Skip spaces
                while (*space == ' ') space++;
                
                if (*space && !fat32_is_initialized()) {
                    writeOut("Error: Filesystem not mounted. Run 'setup' then 'part'.\n");
                } else if (*space) {
                    return prog_mv(src, space);
                } else {
                    writeOut("Usage: mv <source> <destination>\n");
                }
            } else {
                writeOut("Usage: mv <source> <destination>\n");
            }
        } else {
            writeOut("Usage: mv <source> <destination>\n");
        }
    }
    else if (cmd[0] != '\0') {
        print("Invalid command: ", cmd, "\n");
    }

    return 0;
}
