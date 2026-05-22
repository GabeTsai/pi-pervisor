#include "aarch32.h"

.section ".text.boot"

.global _start
_start:
    cpsid if
    @ park all cores except core 0
    mrc p15, 0, r0, c0, c0, 5
    and r0, r0, #0x3
    cmp r0, #0
    bne park_core

    @ if in Hyp mode, leave it 
    mrs r0, cpsr
    and r0, r0, #0x1F
    cmp r0, #0x1A
    bne 1f

    @ we are in Hyp mode
    ldr r0, =1f
    msr elr_hyp, r0
    
    mov r0, #(0x13 | 0xc0) @ Supervisor mode, IRQ/FIQ masked
    msr spsr_cxsf, r0
    
    eret @ exit hyp mode with the two values above

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
    @ set IRQ stack
    cps #0x12 @ IRQ mode
    ldr sp, =IRQ_STACK_TOP

    @ SVC mode
    cps #0x13
    
    @ set VBAR to interrupt table
    ldr r0, =_interrupt_table
    mcr p15, 0, r0, c12, c0, 0
    isb

    @ enable IRQs 
    cpsie i

    bl main
    bl clean_reboot

halt: 
    wfe
    b halt

.p2align 5
.global _interrupt_table
_interrupt_table:
    ldr pc, _reset_h
    ldr pc, _undefined_instruction_vector_h
    ldr pc, _software_interrupt_vector_h
    ldr pc, _prefetch_abort_vector_h
    ldr pc, _data_abort_vector_h
    ldr pc, _unused_handler_h
    ldr pc, _interrupt_vector_h
    ldr pc, _fast_interrupt_vector_h

_reset_h:                           .word   _reset_
_undefined_instruction_vector_h:    .word   undefined_instruction_vector
_software_interrupt_vector_h:       .word   software_interrupt_vector
_prefetch_abort_vector_h:           .word   prefetch_abort_vector
_data_abort_vector_h:               .word   data_abort_vector
_unused_handler_h:                  .word   _reset_
_interrupt_vector_h:                .word   interrupt_vector
_fast_interrupt_vector_h:           .word   fast_interrupt_vector

_reset_:
    b _start
