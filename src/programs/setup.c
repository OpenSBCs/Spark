#include <package.h>
#include "strings.h"
#include <io/print.h>
#include <drivers/fat32Driver.h>

// Preload menu selector
void SelectParition(void);

void prog_setup(void) {
    char buffer[64]; // predefine input data on RAM
    char prompt[] = "(SSW) > ";

    int running = 1;

    print("Spark Setup Wizard\n");

    while (running == 1) {
        print(prompt);
        readline(buffer, sizeof(buffer));
        if (strcmp(buffer, "exit") == 0) {
            running--;
        }
        else if (strcmp(buffer, "part") == 0) {
            // Show the interactive partition selection menu
            SelectParition();
        }
        else if (strcmp(buffer, "help") == 0 || strcmp(buffer, "") == 0) {
            print(setup_help, "\n");
        }
        else if (strcmp(buffer, "spm") == 0 || strcmp(buffer, "snatch") == 0) {
            print("snatch (Spark Package Manager) comming soon\n");
        }
        else {
            print("unknown command: ", buffer, "\n");
        }
    }

    return;
};
