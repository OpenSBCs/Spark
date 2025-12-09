#include "package.h"

void exit(void) {
    // Use ARM semihosting to exit QEMU
    // SYS_EXIT = 0x18, ADP_Stopped_ApplicationExit = 0x20026
    __asm__ volatile (
        "mov r0, #0x18\n"       // SYS_EXIT
        "ldr r1, =#0x20026\n"   // ADP_Stopped_ApplicationExit
        "svc #0x123456\n"       // ARM semihosting call
    );

    // Fallback halt
    while (1) {
    }
}
