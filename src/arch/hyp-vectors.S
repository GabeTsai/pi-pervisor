#include "aarch32.h"

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
hyp_undef:
hyp_prefetch_abort:
hyp_data_abort:
hyp_trap:
hyp_irq:
hyp_fiq:
    b hyp_halt

hyp_hvc_from_hyp:
    push {r0-r12, lr}

    mrc p15, 4, r0, c5, c2, 0 @ read HSR 
    mrs r1, elr_hyp
    mrs r2, spsr_hyp
    bl hyp_dump_exception_state

    pop {r0-r12, lr}
    eret

hyp_halt:
    wfe
    b hyp_halt
