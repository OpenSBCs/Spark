#include "package.h"
#include "print.h"
#include "functions/drivers/fat32Driver.h"

void setup_process(void);  // Forward declaration

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
            "    cat <file>    Display file contents\n"
            "    exists <path> Check if file/dir exists\n"
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
    else if (startsWith(cmd, "cat ")) {
        if (!fat32_is_initialized()) {
            writeOut("Error: Filesystem not mounted. Run 'setup' then 'part'.\n");
        } else {
            const char *path = get_arg(cmd, "cat");
            if (path) {
                static char file_buffer[4096];
                int bytes = fat32_read_file(path, file_buffer, sizeof(file_buffer) - 1);
                if (bytes >= 0) {
                    file_buffer[bytes] = '\0';
                    writeOut(file_buffer);
                    writeOut("\n");
                } else {
                    writeOut("Error: Could not read file\n");
                }
            } else {
                writeOut("Usage: cat <filename>\n");
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
    else if (cmd[0] != '\0') {
        print("Invalid command: ", cmd, "\n");
    }

    return 0;
}
