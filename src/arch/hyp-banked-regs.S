#include "hyp-regs.h"

.global hyp_save_banked_regs
.global hyp_load_banked_regs

.section ".text"
@ r0 contains HypBankedRegs pointer
hyp_save_banked_regs:
    mrs r1, sp_usr
    str r1, [r0, #HYP_BANKED_SP_USR]

    mrs r1, sp_svc
    str r1, [r0, #HYP_BANKED_SP_SVC]

    mrs r1, lr_svc
    str r1, [r0, #HYP_BANKED_LR_SVC]

    mrs r1, spsr_svc
    str r1, [r0, #HYP_BANKED_SPSR_SVC]

    mrs r1, sp_irq
    str r1, [r0, #HYP_BANKED_SP_IRQ]

    mrs r1, lr_irq
    str r1, [r0, #HYP_BANKED_LR_IRQ]

    mrs r1, spsr_irq
    str r1, [r0, #HYP_BANKED_SPSR_IRQ]

    bx lr

hyp_load_banked_regs:
    ldr r1, [r0, #HYP_BANKED_SP_USR]
    msr sp_usr, r1

    ldr r1, [r0, #HYP_BANKED_SP_SVC]
    msr sp_svc, r1

    ldr r1, [r0, #HYP_BANKED_LR_SVC]
    msr lr_svc, r1

    ldr r1, [r0, #HYP_BANKED_SPSR_SVC]
    msr spsr_svc, r1

    ldr r1, [r0, #HYP_BANKED_SP_IRQ]
    msr sp_irq, r1

    ldr r1, [r0, #HYP_BANKED_LR_IRQ]
    msr lr_irq, r1

    ldr r1, [r0, #HYP_BANKED_SPSR_IRQ]
    msr spsr_irq, r1

    bx lr