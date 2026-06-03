#include <stdint.h>
#include "hyp-exceptions.h"
#include "printk.h"
#include "hyp-regs.h"
#include "check.h" 
#include "panic.h"
#include "lower-hvc-test.h"
#include "aarch32.h"

const int verbose = 1;

static const char *hsr_ec_name(uint32_t ec) {
    switch (ec) {
        case HSR_EC_UNKNOWN:
            return "unknown";
        case HSR_EC_WFI_WFE:
            return "wfi/wfe";
        case HSR_EC_CP15_MCR_MRC:
            return "cp15 mcr/mrc";
        case HSR_EC_CP15_MCRR_MRRC:
            return "cp15 mcrr/mrrc";
        case HSR_EC_CP14_MCR_MRC:
            return "cp14 mcr/mrc";
        case HSR_EC_LDC_STC:
            return "ldc/stc";
        case HSR_EC_FP_ASIMD:
            return "fp/asimd";
        case HSR_EC_VMRS_ID_TRAP:
            return "vmrs id trap";
        case HSR_EC_CP14_MRRC:
            return "cp14 mrrc";
        case HSR_EC_ILLEGAL_EXCEPTION_RETURN_A32:
            return "illegal exception return";
        case HSR_EC_SVC_A32:
            return "svc a32";
        case HSR_EC_HVC_A32:
            return "hvc a32";
        case HSR_EC_SMC_A32:
            return "smc a32";
        case HSR_EC_PREFETCH_ABORT_LOWER:
            return "prefetch abort lower";
        case HSR_EC_PREFETCH_ABORT_SAME:
            return "prefetch abort same";
        case HSR_EC_PC_ALIGNMENT_FAULT:
            return "pc alignment fault";
        case HSR_EC_DATA_ABORT_LOWER:
            return "data abort lower";
        case HSR_EC_DATA_ABORT_SAME:
            return "data abort same";
        default:
            return "reserved/unknown";
    }
}

static uint32_t hvc_imm(uint32_t hsr) {
    return HSR_ISS(hsr) & 0xffff;
}

void hyp_dump_exception_state(HypExceptState *hyp_state) { 
    trace("-------- HYP EXCEPTION STATE --------\n");
    trace("EXCEPTION TYPE: %d\n", hyp_state->exception_type);
    if (verbose) { 
        trace("HSR: %p\n", hyp_state->hsr);
        trace("HSR EC: %d (%s)\n", HSR_EC(hyp_state->hsr), hsr_ec_name(HSR_EC(hyp_state->hsr)));
        trace("HSR ISS: %p\n", HSR_ISS(hyp_state->hsr));
        trace("ELR_HYP: %p\n", hyp_state->elr_hyp);
        trace("SPSR_HYP: %b\n", hyp_state->spsr_hyp);
        trace("HDFAR: %p\n", hyp_state->hdfar);
        trace("HIFAR: %p\n", hyp_state->hifar);
        trace("HPFAR: %p\n", hyp_state->hpfar);
    } 
    trace("-------------------------------------\n");
}

// EC values are from 0 to 63 so we need 64-bit masks
static const uint64_t valid_hsr_ec[] = {
    [HYP_EXCEPTION_UNKNOWN] =
        EC_BIT(HSR_EC_UNKNOWN),

    [HYP_EXCEPTION_UNDEF_INSTR] =
        EC_BIT(HSR_EC_UNKNOWN),

    [HYP_EXCEPTION_HVC_FROM_HYP] =
        EC_BIT(HSR_EC_HVC_A32),

    [HYP_EXCEPTION_PREFETCH_ABORT] =
        EC_BIT(HSR_EC_PREFETCH_ABORT_LOWER) |
        EC_BIT(HSR_EC_PREFETCH_ABORT_SAME) |
        EC_BIT(HSR_EC_PC_ALIGNMENT_FAULT),

    [HYP_EXCEPTION_DATA_ABORT] =
        EC_BIT(HSR_EC_DATA_ABORT_LOWER) |
        EC_BIT(HSR_EC_DATA_ABORT_SAME),

    [HYP_EXCEPTION_LOWER_SYNC] =
        EC_BIT(HSR_EC_HVC_A32) |
        EC_BIT(HSR_EC_WFI_WFE) |
        EC_BIT(HSR_EC_CP15_MCR_MRC) |
        EC_BIT(HSR_EC_CP15_MCRR_MRRC) |
        EC_BIT(HSR_EC_CP14_MCR_MRC) |
        EC_BIT(HSR_EC_CP14_MRRC) |
        EC_BIT(HSR_EC_FP_ASIMD),
};

bool hsr_ec_valid(uint32_t hsr, HypExceptType except_type) { 
    uint8_t hsr_ec = HSR_EC(hsr);
    if (except_type < 0 ||
        (uint32_t)except_type >= sizeof(valid_hsr_ec) / sizeof(valid_hsr_ec[0])) {
        return false;
    }
    return (valid_hsr_ec[except_type] & EC_BIT(hsr_ec)) != 0;
}

HypExceptAction hyp_handle_exception(HypExceptState *hyp_state) { 
    switch (hyp_state->exception_type) { 
        case HYP_EXCEPTION_UNDEF_INSTR:
            return handle_undef_instr(hyp_state);
        case HYP_EXCEPTION_UNKNOWN:
            return handle_unknown(hyp_state);
        case HYP_EXCEPTION_HVC_FROM_HYP:
            return handle_hvc_from_hyp(hyp_state);
        case HYP_EXCEPTION_PREFETCH_ABORT:
            return handle_prefetch_abort(hyp_state);
        case HYP_EXCEPTION_DATA_ABORT:
            return handle_data_abort(hyp_state);
        case HYP_EXCEPTION_LOWER_SYNC:
            return handle_lower_sync(hyp_state);
        case HYP_EXCEPTION_IRQ:
            return handle_irq(hyp_state);
        case HYP_EXCEPTION_FIQ:
            return handle_fiq(hyp_state);
        default:
            hyp_dump_exception_state(hyp_state);
            return HYP_ACTION_HALT;
    }
}

HypExceptAction handle_hvc_from_hyp(HypExceptState *hyp_state) { 
    trace("HVC from Hyp\n");
    assert(hsr_ec_valid(hyp_state->hsr, HYP_EXCEPTION_HVC_FROM_HYP), "HSR EC mismatch");
    trace("HVC immediate: %p\n", hvc_imm(hyp_state->hsr));
    hyp_dump_exception_state(hyp_state);
    return HYP_ACTION_RETURN;
}

HypExceptAction handle_hvc_from_lower(HypExceptState *hyp_state) { 
    trace("HVC from lower level mode\n");
    assert(HSR_EC(hyp_state->hsr) == HSR_EC_HVC_A32, "expected lower-mode HVC");
    assert((hyp_state->spsr_hyp & CPSR_PEMODE_MASK) == CPSR_PEMODE_SVC, "expected SVC mode");
    assert(hyp_state->r[2] == 0x12345678, "expected test value");
    
    switch (hyp_state->r[0]) {
        case LOWER_HVC_TEST_PING:
            assert(hsr_iss_imm16(hyp_state->hsr) == LOWER_HVC_TEST_PING_IMM, "bad HVC immediate");
            trace("lower HVC saved CPSR: %p\n", hyp_state->r[1]);
            hyp_state->r[0] = LOWER_HVC_TEST_RETURN_MAGIC;
            return HYP_ACTION_RETURN;
        case LOWER_HVC_TEST_PASS:
            trace("lower HVC test passed\n");
            return HYP_ACTION_HALT;
        case LOWER_HVC_TEST_FAIL:
            trace("lower HVC test failed\n");
            hyp_dump_exception_state(hyp_state);
            return HYP_ACTION_HALT;
        default:
            trace("unknown lower HVC test event: %p\n", hyp_state->r[0]);
            hyp_dump_exception_state(hyp_state);
            return HYP_ACTION_HALT;
    }
    
}

HypExceptAction handle_undef_instr(HypExceptState *hyp_state) { 
    trace("Undefined instruction in Hyp\n");
    assert(hsr_ec_valid(hyp_state->hsr, HYP_EXCEPTION_UNDEF_INSTR), "HSR EC mismatch");
    hyp_dump_exception_state(hyp_state);
    return HYP_ACTION_HALT;
}

HypExceptAction handle_unknown(HypExceptState *hyp_state) { 
    trace("Unknown exception in Hyp\n");
    assert(hsr_ec_valid(hyp_state->hsr, HYP_EXCEPTION_UNKNOWN), "HSR EC mismatch");
    hyp_dump_exception_state(hyp_state);
    return HYP_ACTION_HALT;
}

HypExceptAction handle_prefetch_abort(HypExceptState *hyp_state) { 
    trace("Prefetch abort in Hyp\n");
    assert(hsr_ec_valid(hyp_state->hsr, HYP_EXCEPTION_PREFETCH_ABORT), "HSR EC mismatch");
    hyp_dump_exception_state(hyp_state);
    return HYP_ACTION_HALT;
}

HypExceptAction handle_data_abort(HypExceptState *hyp_state) { 
    trace("Data abort in Hyp\n");
    assert(hsr_ec_valid(hyp_state->hsr, HYP_EXCEPTION_DATA_ABORT), "HSR EC mismatch");
    hyp_dump_exception_state(hyp_state);
    return HYP_ACTION_HALT;
}

HypExceptAction handle_lower_sync(HypExceptState *hyp_state) { 
    trace("Trap in Hyp\n");
    assert(hsr_ec_valid(hyp_state->hsr, HYP_EXCEPTION_LOWER_SYNC), "HSR EC invalid");
    switch (HSR_EC(hyp_state->hsr)) { 
        case HSR_EC_HVC_A32: 
            return handle_hvc_from_lower(hyp_state);
        case HSR_EC_WFI_WFE:
        case HSR_EC_CP15_MCR_MRC:
        default:
            hyp_dump_exception_state(hyp_state);
            return HYP_ACTION_HALT;
    }
}

// this is temporary, not doing IRQs rn
HypExceptAction handle_irq(HypExceptState *hyp_state) { 
    printk("Unexpected IRQ taken to Hyp\n");
    hyp_dump_exception_state(hyp_state);
    return HYP_ACTION_HALT;
}

// probably won't do fiqs for this project
HypExceptAction handle_fiq(HypExceptState *hyp_state) { 
    printk("Unexpected FIQ taken to Hyp\n");
    hyp_dump_exception_state(hyp_state);
    return HYP_ACTION_HALT;
}
