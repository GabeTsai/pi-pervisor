#include "hv/hypercall.h"
#include "hv/scheduler.h"
#include "hv/virq.h"
#include "panic.h"
#include "platform.h"
#include "timer.h"
#include "uart.h"

HypExceptAction hyp_handle_guest_hypercall(HypExceptState *s) { 
    HvVcpu *vcpu;
    int virq;
    int virq_complete;
    switch (s->r[0]) { 
    case HYPERCALL_GET_ABI_VERSION:
        s->r[0] = HYPERCALL_ABI_VERSION;
        return HYP_ACTION_RETURN;
    case HYPERCALL_GET_FEATURES:
        s->r[0] = HYP_FEATURE_BASE | HYP_FEATURE_VIRQ | HYP_FEATURE_TIMER | HYP_FEATURE_UART;
        return HYP_ACTION_RETURN;
    case HYPERCALL_EXIT:
        vcpu = hv_scheduler_get_current(&scheduler);
        if (vcpu == 0) { 
            return HYP_ACTION_HALT;
        } 

        trace("guest exiting, status: %d\n", s->r[1]);
        // if we don't disable the timer, the hyp scan for earliest available virtual timer deadline
        // we see that the current guest's deadline is expired, and will repeatedly rearm it, leading to a hang
        hv_vcpu_timer_disable(vcpu);
        vcpu->state = HV_VCPU_EXITED;
        hv_vtimer_rearm_physical(&scheduler);
        return hv_scheduler_advance(&scheduler, s);
    case HYPERCALL_YIELD:
        return hv_scheduler_yield(&scheduler, s);
    case HYPERCALL_PUTCHAR:
        UART_Put8(s->r[1]);
        s->r[0] = HYPERCALL_SUCCESS;
        return HYP_ACTION_RETURN;
    case HYPERCALL_GET_HYP_IRQ_COUNT:
        s->r[0] = hyp_get_irq_count();
        return HYP_ACTION_RETURN;
    case HYPERCALL_VIRQ_CLAIM:
        vcpu = hv_scheduler_get_current(&scheduler);
        virq = hv_virq_claim(vcpu);
        hv_virq_sync(vcpu);
        // naming is a bit confusing here - compare value is the virq-specific error code. 
        // return value is the final hypercall return code.
        if (virq == HV_VIRQ_ERR_NO_PENDING) { 
            s->r[0] = HYPERCALL_ERR_NO_PENDING;
        } else if (virq < 0) {
            s->r[0] = HYPERCALL_ERR_INVALID;
        } else {
            s->r[0] = virq;
        }
        return HYP_ACTION_RETURN;
    case HYPERCALL_VIRQ_COMPLETE:
        vcpu = hv_scheduler_get_current(&scheduler);
        virq_complete = hv_virq_complete(vcpu, s->r[1]);
        hv_virq_sync(vcpu);
        s->r[0] = virq_complete == 0 ? HYPERCALL_SUCCESS : HYPERCALL_ERR_INVALID;
        return HYP_ACTION_RETURN;
    case HYPERCALL_TIMER_GET_TICKS:
        s->r[0] = TIM_SYS_Get_Ticks();
        return HYP_ACTION_RETURN;
    case HYPERCALL_TIMER_GET_FREQ:
        vcpu = hv_scheduler_get_current(&scheduler);
        if (vcpu == 0 || hv_scheduler_is_idle_vcpu(vcpu)) {
            s->r[0] = 0;
            return HYP_ACTION_RETURN;
        }

        s->r[0] = hv_vcpu_timer_get_frequency(vcpu);
        return HYP_ACTION_RETURN;
    case HYPERCALL_TIMER_DELAY_TICKS:
        vcpu = hv_scheduler_get_current(&scheduler);
        if (vcpu == 0 || hv_scheduler_is_idle_vcpu(vcpu)) {
            s->r[0] = HYPERCALL_ERR_INVALID;
            return HYP_ACTION_RETURN;
        }

        if (s->r[1] == 0) {
            s->r[0] = HYPERCALL_SUCCESS;
            return HYP_ACTION_RETURN;
        }

        hv_vcpu_timer_delay(vcpu, TIM_SYS_Get_Ticks(), s->r[1]);
        hv_vtimer_rearm_physical(&scheduler);
        s->r[0] = HYPERCALL_SUCCESS;
        return hv_scheduler_advance(&scheduler, s);
    default:
        s->r[0] = HYPERCALL_ERR_UNKNOWN;
        return HYP_ACTION_HALT;
    }
}
