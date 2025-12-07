#include "package.h"
#include "io/shell.h"

void kernel_main(void) {
    initGraphics();
    writeOut("Hello from spark!\n\n");

    sh_start();

    exit();
}
