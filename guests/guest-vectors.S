// guests/guest-vectors.S
#include "hv/hypercall.h"

.section ".text.vectors"

.p2align 5
.global guest_vector_table
guest_vector_table:
    ldr pc, _guest_reset_h
    ldr pc, _guest_undef_h
    ldr pc, _guest_svc_h
    ldr pc, _guest_prefetch_abort_h
    ldr pc, _guest_data_abort_h
    ldr pc, _guest_reserved_h
    ldr pc, _guest_irq_h
    ldr pc, _guest_fiq_h

_guest_reset_h:          .word guest_unhandled
_guest_undef_h:          .word guest_unhandled
_guest_svc_h:            .word guest_unhandled
_guest_prefetch_abort_h: .word guest_unhandled
_guest_data_abort_h:     .word guest_unhandled
_guest_reserved_h:       .word guest_unhandled
_guest_irq_h:            .word guest_irq
_guest_fiq_h:            .word guest_unhandled

.global guest_irq
guest_irq:
    sub lr, lr, #4
    push {r0-r12, lr}
    bl guest_irq_handler
    pop {r0-r12, lr}
    subs pc, lr, #0

.weak guest_irq_handler
guest_irq_handler:
    b guest_unhandled

.global guest_unhandled
guest_unhandled:
    mov r0, #HYPERCALL_EXIT
    mov r1, #HYP_GUEST_EXIT_FAILURE
    hvc #HYPERCALL_HVC_IMM
