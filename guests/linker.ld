ENTRY(_guest_start)

SECTIONS
{
    . = 0x20000;

    .text : {
        *(.text.boot)
        *(.text*)
    }

    .rodata : { *(.rodata*) }
    .data : { *(.data*) }

    .bss : {
        __guest_bss_start__ = .;
        *(.bss*)
        *(COMMON)
        __guest_bss_end__ = .;
    }

    . = ALIGN(8);
    __guest_stack_bottom__ = .;
    . += 4096;
    __guest_stack_top__ = .;

    . = ALIGN(8);
    __guest_irq_stack_bottom__ = .;
    . += 1024;
    __guest_irq_stack_top__ = .;
}