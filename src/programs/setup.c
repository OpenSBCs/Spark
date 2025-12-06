#include "../package.h"
#include "strings.h"
#include "../print.h"

void setup_process(void) {
    char buffer[100]; // predefine input data on RAM
    char prompt[] = "(SSW) > ";

    int running = 1;

    print("Spark Setup Wizard\n");

    while (running == 1) {
        print(prompt);
        readLine(buffer, sizeof(buffer));
        if (strcmp(buffer, "exit") == 0) {
            running--;
        }
        else if (strcmp(buffer, "part") == 0) {
            print("Partion setup comming soon\n");
        }
        else if (strcmp(buffer, "help") == 0 || strcmp(buffer, "") == 0) {
            print(setup_help, "\n");
        }
        else if (strcmp(buffer, "spm") == 0) {
            print("snatch (Spark Package Manager) comming soon\n");
        }
        else {
            print("unknown command: ", buffer, "\n");
        }
    }
    
    return;
};