ENTRY(_start)

MEMORY {
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 256K
    RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 64K
}

SECTIONS {
    .text : {
        KEEP(*(.vectors))
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    _data_load = LOADADDR(.data);

    .data : {
        . = ALIGN(4);
        _data_start = .;
        *(.data*)
        . = ALIGN(4);
        _data_end = .;
    } > RAM AT> FLASH

    /* ΕΔΩ Η ΠΡΟΣΘΗΚΗ: Μνήμη που επιβιώνει από reset */
    .noinit (NOLOAD) : {
        . = ALIGN(4);
        *(.noinit*)
        . = ALIGN(4);
    } > RAM

    .bss : {
        . = ALIGN(4);
        _bss_start = .;
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
        __bss_end__ = .;
    } > RAM

    . = ALIGN(8);
    end = .;
    _end = .;
    __end__ = .;

    _stack_top = ORIGIN(RAM) + LENGTH(RAM);
}