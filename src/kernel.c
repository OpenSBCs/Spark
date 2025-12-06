/*
 * ============================================================================
 *                              SPARK KERNEL
 * ============================================================================
 *
 * This is the main kernel file for Spark OS.
 * It handles the command-line interface and processes user commands.
 *
 * Author: syntaxMORG0 and Samuraien2
 *
 * ============================================================================
 */

#include "package.h"
#include "shell.h"

/*
 * kernel_main - The main entry point of the kernel
 *
 * This function:
 * 1. Initializes the graphics/display
 * 2. Shows a welcome message
 * 3. Runs a loop that reads commands and executes them
 * 4. Exits when the user types "exit"
 */
void kernel_main(void) {
    // Buffer to store the user's input
    char input_buf[128];

    // Flag to keep the kernel running (1 = running, 0 = exit)
    int running = 1;

    initGraphics();

    writeOut("Hello from spark!\n\n");

    /* ------------------------------------------------------------------------
     * MAIN COMMAND LOOP
     * ------------------------------------------------------------------------ */
    while (running) {
        // Show prompt and read user input
        writeOut("> ");
        readLine(input_buf, sizeof(input_buf));

        // Execute user input
        int ret = sh_exec(input_buf);
        if (ret == 66) {
            running = 0;
        }
    }

    exit();
}
