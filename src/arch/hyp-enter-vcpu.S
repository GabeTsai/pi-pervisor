#include "hyp-exceptions-layout.h"

.section ".text"

.global hyp_enter_vcpu_context
.type hyp_enter_vcpu_context, %function

@ r0 = HypExceptState *
@ r1 = HypBankedRegs *
hyp_enter_vcpu_context:
    mov r4, r0

    mov r0, r1
    bl hyp_load_banked_regs

    ldr r1, [r4, #HYP_STATE_ELR_HYP]
    msr elr_hyp, r1

    ldr r1, [r4, #HYP_STATE_SPSR_HYP]
    msr spsr_hyp, r1

    ldr lr, [r4, #HYP_STATE_LR]
    ldmia r4, {r0-r12}

    eret

.size hyp_enter_vcpu_context, . - hyp_enter_vcpu_context
