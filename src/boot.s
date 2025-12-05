/* boot.s - Simple ARM Bootloader */
.global _start
.section .text
_start:
    /* Set up the stack pointer */
    ldr sp, =stack_top

    /* Jump to kernel main function */
    bl kernel_main

halt:
    b halt

/* Define stack */
.section .bss
.align 3
stack:
    .space 0x1000
stack_top:
