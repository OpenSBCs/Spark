#include "package.h"
#include "strings.h"
#include "print.h"

void setup_process(void);  // Forward declaration

int sh_exec(const char *cmd) {
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "") == 0) {
        writeOut(HELP_TEXT);
        newline(1);
    }
    else if (strcmp(cmd, "credits") == 0) {
        print("Spark is made and developed by syntaxMORG0 and Samuraien2\n");
    }
    else if (strcmp(cmd, "repo") == 0) {
        print("View the spark project here: https://github.com/syntaxMORG0/Spark\n");
    }
    else if (strcmp(cmd, "exit") == 0) {
        return 66;
    }
    else if (strcmp(cmd, "setup") == 0) {
        setup_process();
    }
    else if (cmd[0] != '\0') {
        print("Invalid command: ", cmd, "\n");
    }

    return 0;
}
