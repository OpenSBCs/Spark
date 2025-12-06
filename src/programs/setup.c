#include "../package.h"
#include "strings.h"

void setup_process(void) {
    char buffer[100]; // predefine input data on RAM
    char prompt[] = "(SSW) > ";

    int running = 1;

    writeOut("Spark Setup Wizard"), newline(1);

    while (running == 1) {
        writeOut(prompt), readLine(buffer, sizeof(buffer));
        if (strcmp(buffer, "exit") == 0) {
            running--;
        }
        else if (strcmp(buffer, "part") == 0) {
            writeOut("Partion setup comming soon"), newline(1);
        }
        else if (strcmp(buffer, "help") == 0 || strcmp(buffer, "") == 0) {
            writeOut(setup_help);
            newline(2);
        }
        else if (strcmp(buffer, "spm") == 0) {
            writeOut("SPM (Spark Package Manager) comming soon"), newline(1);
        }
        else {
            writeOut("unknown command: "), writeOut(buffer), newline(1);
        }
    }
    
    return;
};