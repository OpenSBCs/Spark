#include "package.h"
#include "strings.h"

void kernel_main(void) {
    char buffer[100];
    
    char prompt[] = "> ";
    int running = 1;
    
    initGraphics();
    
    writeOut(WELCOME_TEXT);
    BreakLine(2);
    
    while (running == 1) {
        writeOut(prompt);
        readLine(buffer, sizeof(buffer));
        
        if (strcmp(buffer, "help") == 0) {
            writeOut(HELP_TEXT);
            BreakLine(2);
        }
        else if (strcmp(buffer, "setup") == 0) {
            setup_prosess();
        }
        else if (strcmp(buffer, "credits") == 0) {
            writeOut(CREDITS);
        }
        else if (strcmp(buffer, "repo") == 0) {
            writeOut(REPO_DETAILS);
        }
        else if (strcmp(buffer, "exit") == 0) {
            running = 0;
        }
        
        else if (buffer[0] != '\0') {
            writeOut(ERR_INVALID_CMD);
            writeOut(buffer);
            BreakLine(1);
        }
    }
    BreakLine(1);
    exit();
}
