#include "../package.h"

void setup_prosess(void) {
    char buffer[100]; // predefine input data on RAM
    char prompt[] = "> ";

    int running = 1;

    writeOut("hello world from setup\ntype --exit to exit setup\n");

    while (running == 1) {
        writeOut(prompt), readLine(buffer, sizeof(buffer));
        if (strcmp(buffer, "--exit") == 0) {
            running--;
        }
    }
    
    return;
};