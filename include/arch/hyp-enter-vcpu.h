#pragma once

#include "hyp-exceptions.h"
#include "hyp-regs.h"

__attribute__((noreturn))
void hyp_enter_vcpu_context(HypExceptState *hyp_state, HypBankedRegs *banked_regs);
