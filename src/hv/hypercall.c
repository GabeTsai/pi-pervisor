#include "hv/hypercall.h"
#include "panic.h"
#include "platform.h"

HypExceptAction hyp_handle_guest_hypercall(HypExceptState *s) { 
    switch (s->r[0]) { 
    case HYPERCALL_GET_ABI_VERSION:
        s->r[0] = HYPERCALL_ABI_VERSION;
        return HYP_ACTION_RETURN;
    case HYPERCALL_GET_FEATURES:
        s->r[0] = HYP_FEATURE_BASE;
        return HYP_ACTION_RETURN;
    case HYPERCALL_EXIT:
        trace("guest exiting, status: %d\n", s->r[1]);
        clean_reboot();
        return HYP_ACTION_HALT;
    case HYPERCALL_YIELD:
        s->r[0] = HYPERCALL_SUCCESS;
        return HYP_ACTION_RETURN;
    default:
        s->r[0] = HYPERCALL_ERR_UNKNOWN;
        return HYP_ACTION_HALT;
    }
}