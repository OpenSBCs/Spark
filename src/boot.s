.global _start
.section .text
_start:
    ldr sp, =stack_top
    bl kernel_main

halt:
    b halt

.section .bss
.align 3
stack:
    .space 0x1000
stack_top:
