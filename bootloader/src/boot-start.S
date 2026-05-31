#include "aarch32.h"
.section ".text.boot"

.globl _start
_start:
    b skip
.space 0x200000-0x8004,0

skip:
    cpsid if

    @ park all cores except core 0
    mrc p15, 0, r0, c0, c0, 5
    and r0, r0, #0x3
    cmp r0, #0
    bne park_core

    @ If firmware entered us in Hyp mode, set SP_hyp and stay there.
    @ Otherwise initialize the current-mode stack.
    mrs r0, cpsr
    and r0, r0, #0x1F
    cmp r0, #0x1A
    beq set_hyp_stack
    ldr sp, =STACK_ADDR
    b 2f

set_hyp_stack:
    ldr sp, =HYP_STACK_TOP

2:
    ldr r0, =__bss_start__
    ldr r1, =__bss_end__
    mov r2, #0
    b clear_bss

park_core:
    cpsid if

wait:
    wfe
    b wait

clear_bss:
    cmp r0, r1
    bge clear_bss_done
    str r2, [r0], #4
    b clear_bss

clear_bss_done:
    bl main

halt:
    wfe
    b halt
