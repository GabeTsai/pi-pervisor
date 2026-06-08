#include "hv/scheduler.h"
#include "hv/hypercall.h"
#include "hv/virq.h"
#include "panic.h"

HvScheduler scheduler;
bool hv_scheduler_verbose;

void hv_scheduler_init(HvScheduler *scheduler) {
    scheduler->runnable_mask = 1;
    scheduler->cur_idx = 0;

    for (uint32_t i = 0; i < HV_MAX_VCPUS; i++) {
        scheduler->vcpus[i].id = i;
        scheduler->vcpus[i].state = HV_VCPU_IDLE;
        scheduler->vcpus[i].virq_pending = 0;
        scheduler->vcpus[i].virq_active = 0;
    }

    scheduler->vcpus[0].state = HV_VCPU_RUNNABLE;
    scheduler->cur_vcpu = &scheduler->vcpus[0];
}

HvVcpu *hv_scheduler_get_current(HvScheduler *scheduler) {
    return scheduler->cur_vcpu;
}

// simple round-robin scheduler. Note that it is possible for it to return the current vCPU
int hv_scheduler_pick_next_idx(HvScheduler *scheduler) {
    uint32_t start = scheduler->cur_idx;

    for (uint32_t offset = 1; offset <= HV_MAX_VCPUS; offset++) {
        uint32_t idx = (start + offset) % HV_MAX_VCPUS;

        if (scheduler->vcpus[idx].state == HV_VCPU_RUNNABLE) {
            return idx;
        }
    }

    return -1;
}

HvVcpu *hv_scheduler_pick_next(HvScheduler *scheduler) {
    int idx = hv_scheduler_pick_next_idx(scheduler);
    if (idx == -1) {
        return 0;
    }
    return &scheduler->vcpus[idx];
}

HypExceptAction hv_scheduler_switch_to(HvScheduler *scheduler, HypExceptState *hyp_state, int next_idx) {
    HvVcpu *prev = scheduler->cur_vcpu;
    HvVcpu *next = &scheduler->vcpus[next_idx];
    uint32_t prev_id = prev == 0 ? 0xffffffff : prev->id;

    if (prev != 0) { 
        hv_vcpu_save(prev, hyp_state);

        if (prev->state == HV_VCPU_RUNNING) { 
            prev->state = HV_VCPU_RUNNABLE;
        }
    }

    if (hv_scheduler_verbose) {
        trace("sched switch prev=%d next=%d elr=%p\n",
               prev_id,
               next->id,
               next->context.hyp_state.elr_hyp);
    }

    next->state = HV_VCPU_RUNNING;
    scheduler->cur_idx = next_idx;
    scheduler->cur_vcpu = next;

    hv_vcpu_load(next, hyp_state);
    // clear HCR.VI if no pending virqs, otherwise set it
    hv_virq_sync(next);
    return HYP_ACTION_RETURN;
}

HypExceptAction hv_scheduler_yield(HvScheduler *scheduler, HypExceptState *hyp_state) {
    // set current vCPU as runnable
    HvVcpu *cur = scheduler->cur_vcpu;
    if (cur == 0) { 
        return HYP_ACTION_RETURN;
    }
    cur->state = HV_VCPU_RUNNABLE;
    
    int next_idx = hv_scheduler_pick_next_idx(scheduler);
    // if no next vCPU is found, just run the current vCPU again
    if (next_idx < 0 || &scheduler->vcpus[next_idx] == cur) { 
        cur->state = HV_VCPU_RUNNING;
        return HYP_ACTION_RETURN;
    }
    return hv_scheduler_switch_to(scheduler, hyp_state, next_idx);
}

HypExceptAction hv_scheduler_advance(HvScheduler *scheduler, HypExceptState *hyp_state) {
    HvVcpu *cur = scheduler->cur_vcpu;
    if (cur == 0) {
        return HYP_ACTION_RETURN;
    }

    if (cur->state == HV_VCPU_RUNNING) {
        cur->state = HV_VCPU_RUNNABLE;
    }

    int next_idx = hv_scheduler_pick_next_idx(scheduler);
    if (next_idx < 0) { 
        cur->state = HV_VCPU_RUNNING;
        return HYP_ACTION_RETURN;
    }
    return hv_scheduler_switch_to(scheduler, hyp_state, next_idx);
}
