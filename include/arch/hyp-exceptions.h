#pragma once

#include "hyp-exceptions-layout.h"

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef int HypExceptAction;

typedef int HypExceptType;

typedef struct { 
    uint32_t r[13];
    uint32_t lr;
    uint32_t hsr;
    uint32_t elr_hyp;
    uint32_t spsr_hyp;
    uint32_t hdfar;
    uint32_t hifar;
    uint32_t hpfar;
    uint32_t exception_type;
} HypExceptState;

// bunch of asserts to make sure layout is correct
_Static_assert(offsetof(HypExceptState, r[0]) == HYP_STATE_R0, "r0 offset mismatch");
_Static_assert(offsetof(HypExceptState, r[1]) == HYP_STATE_R1, "r1 offset mismatch");
_Static_assert(offsetof(HypExceptState, lr) == HYP_STATE_LR, "lr offset mismatch");
_Static_assert(offsetof(HypExceptState, hsr) == HYP_STATE_HSR, "hsr offset mismatch");
_Static_assert(offsetof(HypExceptState, elr_hyp) == HYP_STATE_ELR_HYP, "elr_hyp offset mismatch");
_Static_assert(offsetof(HypExceptState, spsr_hyp) == HYP_STATE_SPSR_HYP, "spsr_hyp offset mismatch");
_Static_assert(offsetof(HypExceptState, hdfar) == HYP_STATE_HDFAR, "hdfar offset mismatch");
_Static_assert(offsetof(HypExceptState, hifar) == HYP_STATE_HIFAR, "hifar offset mismatch");
_Static_assert(offsetof(HypExceptState, hpfar) == HYP_STATE_HPFAR, "hpfar offset mismatch");
_Static_assert(offsetof(HypExceptState, exception_type) == HYP_STATE_EXCEPTION_TYPE, "exception_type offset mismatch");
_Static_assert(sizeof(HypExceptState) == HYP_STATE_SIZE, "HypExceptState size mismatch");

void hyp_dump_exception_state(HypExceptState *hyp_state);
bool hsr_ec_valid(uint32_t hsr, HypExceptType except_type);

HypExceptAction hyp_handle_exception(HypExceptState *hyp_state);

HypExceptAction handle_unknown(HypExceptState *hyp_state);
HypExceptAction handle_undef_instr(HypExceptState *hyp_state);
HypExceptAction handle_hvc_from_hyp(HypExceptState *hyp_state);
HypExceptAction handle_prefetch_abort(HypExceptState *hyp_state);
HypExceptAction handle_data_abort(HypExceptState *hyp_state);
HypExceptAction handle_trap(HypExceptState *hyp_state);
HypExceptAction handle_irq(HypExceptState *hyp_state);
HypExceptAction handle_fiq(HypExceptState *hyp_state);


#endif
