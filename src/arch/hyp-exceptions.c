#include <stdint.h>
#include "hyp-exceptions.h"
#include "printk.h"
#include "hyp-regs.h"
#include "check.h" 

const int verbose = 1;

void hyp_dump_exception_state(HypExceptState *hyp_state) { 
    printk("-------- HYP EXCEPTION STATE --------\n");
    printk("EXCEPTION TYPE: %d\n", hyp_state->exception_type);
    if (verbose) { 
        printk("HSR: %p\n", hyp_state->hsr);
        printk("ELR_HYP: %p\n", hyp_state->elr_hyp);
        printk("SPSR_HYP: %b\n", hyp_state->spsr_hyp);
        printk("HDFAR: %p\n", hyp_state->hdfar);
        printk("HIFAR: %p\n", hyp_state->hifar);
        printk("HPFAR: %p\n", hyp_state->hpfar);
    } 
    
    printk("-------------------------------------\n");
}

bool check_hsr_ec(uint32_t hsr, HypExceptType exception_type) { 
    uint32_t ec = HSR_EC(hsr);
    switch (exception_type) { 
        case HYP_EXCEPTION_HVC_FROM_HYP:
            return ec == HSR_EC_HVC_A32;
        case HYP_EXCEPTION_PREFETCH_ABORT:
            return ec == HSR_EC_PREFETCH_ABORT_LOWER || ec == HSR_EC_PREFETCH_ABORT_SAME;
        case HYP_EXCEPTION_DATA_ABORT:
            return ec == HSR_EC_DATA_ABORT_LOWER || ec == HSR_EC_DATA_ABORT_SAME;
        default:
            return false;
    }
}

HypExceptAction hyp_handle_exception(HypExceptState *hyp_state) { 
    switch (hyp_state->exception_type) { 
        case HYP_EXCEPTION_HVC_FROM_HYP:
            return handle_hvc_from_hyp(hyp_state);
        default:
            hyp_dump_exception_state(hyp_state);
            return HYP_ACTION_HALT;
    }
}

HypExceptAction handle_hvc_from_hyp(HypExceptState *hyp_state) { 
    printk("HVC from Hyp\n");
    assert(check_hsr_ec(hyp_state->hsr, HYP_EXCEPTION_HVC_FROM_HYP), "HSR EC mismatch");
    hyp_dump_exception_state(hyp_state);
    return HYP_ACTION_RETURN;
}
