#include "package.h"
#include "print.h"

void setup_process(void);  // Forward declaration

int sh_exec(const char *cmd) {
    if (strcmp(cmd, "help") == 0) {
        print(
            "COMMANDS\n"
            "  help      Show this help menu\n"
            "  about     Show info about Spark\n"
            "  exit      Shutdown Spark\n"
            "  setup     Run setup wizard\n"
            "\n"
        );
    }
    else if (strcmp(cmd, "about") == 0) {
        print(
            "Spark v1.0 - Developed by syntaxMORG0 and Samuraien2\n"
            "Github Repo: https://github.com/syntaxMORG0/Spark\n"
        );
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
