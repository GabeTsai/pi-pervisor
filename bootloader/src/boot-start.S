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

    @ leave Hyp mode before running the C bootloader
    mrs r0, cpsr
    and r0, r0, #0x1F
    cmp r0, #0x1A
    bne 1f

    ldr r0, =1f
    msr elr_hyp, r0

    mov r0, #(0x13 | 0xc0) @ Supervisor mode, IRQ/FIQ masked
    msr spsr_cxsf, r0

    eret

1:
    ldr sp, =STACK_ADDR

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
