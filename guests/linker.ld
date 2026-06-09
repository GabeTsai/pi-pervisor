ENTRY(_guest_start)

PROVIDE(GUEST_BASE = 0x20000);
PROVIDE(GUEST_SLOT_SIZE = 0x10000);

SECTIONS
{
    . = GUEST_BASE;

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

    ASSERT(. <= GUEST_BASE + GUEST_SLOT_SIZE, "guest image exceeds GUEST_SLOT_SIZE")
}
