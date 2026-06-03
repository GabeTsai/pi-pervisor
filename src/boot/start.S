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

    @ If the bootloader handed off in Hyp mode, stay there and keep using
    @ the active SP_hyp for now. Otherwise initialize the current-mode stack.
    mrs r0, cpsr
    and r0, r0, #0x1F
    cmp r0, #0x1A
    beq in_hyp

    ldr sp, =STACK_ADDR
    b 1f

in_hyp:
    ldr sp, =HYP_STACK_TOP

    ldr r0, =_hyp_vector_table
    mcr p15, 4, r0, c12, c0, 0 @ set HVBAR to hyp vector table
    isb

    mov r0, #HCR_SWIO
    mcr p15, 4, r0, c1, c1, 0 @ clear HCR controls, preserving Cortex-A53 RES1 SWIO
    isb

1:
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

.p2align 5
.global _hyp_vector_table
_hyp_vector_table:
    ldr pc, _hyp_reset_h
    ldr pc, _hyp_undef_h
    ldr pc, _hyp_hvc_h
    ldr pc, _hyp_prefetch_abort_h
    ldr pc, _hyp_data_abort_h
    ldr pc, _hyp_lower_sync_h
    ldr pc, _hyp_irq_h
    ldr pc, _hyp_fiq_h

_hyp_reset_h:          .word hyp_unknown
_hyp_undef_h:          .word hyp_undef
_hyp_hvc_h:            .word hyp_hvc_from_hyp
_hyp_prefetch_abort_h: .word hyp_prefetch_abort
_hyp_data_abort_h:     .word hyp_data_abort
_hyp_lower_sync_h:     .word hyp_lower_sync
_hyp_irq_h:            .word hyp_irq
_hyp_fiq_h:            .word hyp_fiq

_reset_:
    b _start
