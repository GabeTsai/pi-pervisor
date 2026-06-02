#include "aarch32.h"
#include "hyp-exceptions-layout.h"
#include "hyp-exceptions-layout.h"

.section ".text"

.global hyp_unknown
.global hyp_undef
.global hyp_hvc_from_hyp
.global hyp_prefetch_abort
.global hyp_data_abort
.global hyp_trap
.global hyp_irq
.global hyp_fiq

hyp_unknown:
    sub sp, sp, #HYP_STATE_SIZE
    str r0, [sp, #HYP_STATE_R0]
    mov r0, #HYP_EXCEPTION_UNKNOWN
    b hyp_except_entry_alloc_frame

hyp_undef:
    sub sp, sp, #HYP_STATE_SIZE
    str r0, [sp, #HYP_STATE_R0]
    mov r0, #HYP_EXCEPTION_UNDEF_INSTR
    b hyp_except_entry_alloc_frame

hyp_prefetch_abort:
    sub sp, sp, #HYP_STATE_SIZE
    str r0, [sp, #HYP_STATE_R0]
    mov r0, #HYP_EXCEPTION_PREFETCH_ABORT
    b hyp_except_entry_alloc_frame

hyp_data_abort:
    sub sp, sp, #HYP_STATE_SIZE
    str r0, [sp, #HYP_STATE_R0]
    mov r0, #HYP_EXCEPTION_DATA_ABORT
    b hyp_except_entry_alloc_frame

hyp_trap:
    sub sp, sp, #HYP_STATE_SIZE
    str r0, [sp, #HYP_STATE_R0]
    mov r0, #HYP_EXCEPTION_TRAP
    b hyp_except_entry_alloc_frame

hyp_irq:
    sub sp, sp, #HYP_STATE_SIZE
    str r0, [sp, #HYP_STATE_R0]
    mov r0, #HYP_EXCEPTION_IRQ
    b hyp_except_entry_alloc_frame

hyp_fiq:
    sub sp, sp, #HYP_STATE_SIZE
    str r0, [sp, #HYP_STATE_R0]
    mov r0, #HYP_EXCEPTION_FIQ
    b hyp_except_entry_alloc_frame

hyp_hvc_from_hyp:
    sub sp, sp, #HYP_STATE_SIZE
    str r0, [sp, #HYP_STATE_R0]
    mov r0, #HYP_EXCEPTION_HVC_FROM_HYP
    b hyp_except_entry_alloc_frame

@ create the exception frame
hyp_except_entry_alloc_frame:
    str r0, [sp, #HYP_STATE_EXCEPTION_TYPE] @ store exception type

    add r0, sp, #HYP_STATE_R1
    stmia r0, {r1-r12} @ save all registers
    
    str lr, [sp, #HYP_STATE_LR]

    mrc p15, 4, r1, c5, c2, 0 @ read HSR 
    str r1, [sp, #HYP_STATE_HSR]

    mrc p15, 4, r1, c5, c2, 0 @ read HSR 
    str r1, [sp, #HYP_STATE_HSR]

    mrs r1, elr_hyp
    str r1, [sp, #HYP_STATE_ELR_HYP]

    mrs r1, spsr_hyp
    str r1, [sp, #HYP_STATE_SPSR_HYP]

    mrc p15, 4, r1, c6, c0, 0 @ read HDFAR
    str r1, [sp, #HYP_STATE_HDFAR]

    mrc p15, 4, r1, c6, c0, 2 @ read HIFAR
    str r1, [sp, #HYP_STATE_HIFAR]

    mrc p15, 4, r1, c6, c0, 4 @ read HPFAR
    str r1, [sp, #HYP_STATE_HPFAR]

    mov r0, sp
    bl hyp_handle_exception

    cmp r0, #HYP_ACTION_RETURN
    bne hyp_halt
    str r1, [sp, #HYP_STATE_ELR_HYP]

    mrs r1, spsr_hyp
    str r1, [sp, #HYP_STATE_SPSR_HYP]

    mrc p15, 4, r1, c6, c0, 0 @ read HDFAR
    str r1, [sp, #HYP_STATE_HDFAR]

    mrc p15, 4, r1, c6, c0, 2 @ read HIFAR
    str r1, [sp, #HYP_STATE_HIFAR]

    mrc p15, 4, r1, c6, c0, 4 @ read HPFAR
    str r1, [sp, #HYP_STATE_HPFAR]

    mov r0, sp
    bl hyp_handle_exception

    cmp r0, #HYP_ACTION_RETURN
    bne hyp_halt

    ldmia sp, {r0-r12}
    ldr lr, [sp, #HYP_STATE_LR]
    add sp, sp, #HYP_STATE_SIZE @ deallocate frame
    ldmia sp, {r0-r12}
    ldr lr, [sp, #HYP_STATE_LR]
    add sp, sp, #HYP_STATE_SIZE @ deallocate frame
    eret

hyp_halt:
    wfe
    b hyp_halt
