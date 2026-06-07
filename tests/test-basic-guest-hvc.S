#include "hv/hypercall.h"

.section ".text"

.global guest_lower_entry

guest_lower_entry:
    mov sp, r0
    bl guest_main

    @ guest_main should terminate through guest_exit().
    @ Returning here means the guest test failed.
    mov r0, #HYPERCALL_EXIT
    mov r1, #HYP_GUEST_EXIT_FAILURE
    hvc #HYPERCALL_HVC_IMM
    bl clean_reboot

.section ".bss"

.balign 8
.global guest_lower_stack
guest_lower_stack:
    .space 4096

.global guest_lower_stack_top
guest_lower_stack_top:
