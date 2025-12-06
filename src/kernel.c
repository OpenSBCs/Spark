#include "package.h"
#include "shell.h"

void kernel_main(void) {
    char input_buf[128];
    int running = 1;

    initGraphics();
    writeOut("Hello from spark!\n\n");

    while (running) {
        writeOut("> ");
        readLine(input_buf, sizeof(input_buf));

        int ret = sh_exec(input_buf);
        if (ret == 66) {
            running = 0;
        }
    }

    exit();
}
