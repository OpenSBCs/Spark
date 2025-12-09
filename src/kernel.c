#include "package.h"
#include "io/shell.h"
// Preload menu (defined in src/Prel.c)
void SelectParition(void);

void kernel_main(void) {
    initGraphics();
    SelectParition();

    writeOut("Hello from spark!\n\n");

    sh_start();

    exit();
}
