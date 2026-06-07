#include "hv/hypercall.h"
#include "aarch32.h"

.section ".text.boot"

.global _guest_start
.extern guest_vector_table

_guest_start:
    ldr sp, =__guest_stack_top__

    ldr r0, =guest_vector_table
    mcr p15, 0, r0, c12, c0, 0 @ set VBAR to guest vector table
    isb

    ldr r0, =__guest_bss_start__
    ldr r1, =__guest_bss_end__
    mov r2, #0

clear_bss:
    cmp r0, r1
    bge clear_done
    str r2, [r0], #4
    b clear_bss

clear_done:
    mrs r0, cpsr
    
    @ In Aarch32, IRQ mode has its own banked stack pointer, as well as SVC
    cps #CPSR_PEMODE_IRQ
    ldr sp, =__guest_irq_stack_top__

    msr cpsr_c, r0 @ back to SVC mode
    ldr sp, =__guest_stack_top__

    bl guest_main

    @ guest_main should terminate through guest_exit(), 
    @ which is a hypercall back to EL2.
    @ Returning here means the guest test failed.
    mov r0, #HYPERCALL_EXIT
    mov r1, #HYP_GUEST_EXIT_FAILURE
    hvc #HYPERCALL_HVC_IMM
