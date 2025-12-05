/* linker.ld - Linker script for ARM kernel */
ENTRY(_start)

SECTIONS
{
    . = 0x10000; /* Start address for versatilepb */

    .text : {
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(.bss*)
        . = ALIGN(4);
    }
}
