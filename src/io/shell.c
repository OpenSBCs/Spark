#include <package.h>
#include <drivers/fat32Driver.h>
#include "print.h"
#include "shell.h"

  // Forward declaration

// External program declarations
int prog_rm(const char *path);
int prog_cat(const char *path);
int prog_cp(const char *src, const char *dst);
int prog_mv(const char *src, const char *dst);
int prog_touch(const char *path);
void prog_setup(void);
void prog_vi(const char *filename);

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

void sh_start(void) {
    char input_buf[128];
    while (1) {
        writeOut("> ");
        readline(input_buf, sizeof(input_buf));

        int ret = sh_exec(input_buf);
        if (ret == 66) {
            break;
        }
    }
}

int sh_exec(const char *cmd) {
    if (strcmp(cmd, "help") == 0) {
        print(
            "COMMANDS\n"
            "  SYSTEM\n"
            "    help          Show this help menu\n"
            "    about         Show info about Spark\n"
            "    exit          Shutdown Spark\n"
            "    setup/ssw     Run setup wizard\n"
            "\n"
            "  FILES\n"
            "    ls [path]     List directory contents\n"
            "    cat <file>    Display file contents\n"
            "    mkf <file>    Create empty file\n"
            "    vi <file>     Edit file with vi editor\n"
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
        prog_setup();
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
    // Builtin: clear
    else if (strcmp(cmd, "clear") == 0) {
        writeOut("\033[2J\033[H");  // ANSI escape codes: clear screen and move cursor to home
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
    else if (startsWith(cmd, "mkf ")) {
        const char *path = get_arg(cmd, "touch");
        if (path) {
            if (!fat32_is_initialized()) {
                writeOut("Error: Filesystem not mounted. Run 'setup' first.\n");
            } else {
                return prog_touch(path);
            }
        } else {
            writeOut("Usage: touch <filename>\n");
        }
    }
    // vi editor
    else if (strcmp(cmd, "vi") == 0) {
        prog_vi((void*)0);  // Open vi with no file
    }
    else if (startsWith(cmd, "vi ")) {
        const char *path = get_arg(cmd, "vi");
        if (path) {
            prog_vi(path);
        } else {
            prog_vi((void*)0);
        }
    }
    else if (cmd[0] != '\0') {
        print("Invalid command: ", cmd, "\n");
    }

    return 0;
}
