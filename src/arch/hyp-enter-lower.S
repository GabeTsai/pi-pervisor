#include "aarch32.h"

.section ".text"

.global hyp_enter_lower_mode
.type hyp_enter_lower_mode, %function

hyp_enter_lower_mode:
    @ r0 = lower-mode entry point
    @ r1 = lower-mode stack top
    @ r2 = CPSR value to restore into lower mode
    msr elr_hyp, r0
    msr spsr_hyp, r2 @ copy PSTATE (cpsr) into spsr_hyp  
    mov r0, r1 @ copy stack top to r0
    eret

.size hyp_enter_lower_mode, . - hyp_enter_lower_mode
