#pragma once

#include <stdbool.h>

#include "vcpu.h"
#include "hyp-exceptions.h"

#define HV_MAX_GUEST_VCPUS 1
#define HV_IDLE_VCPU_IDX HV_MAX_GUEST_VCPUS
#define HV_MAX_VCPUS (HV_MAX_GUEST_VCPUS + 1)
#define HV_IDLE_VCPU_ID 0xffffffffu

typedef struct HvScheduler { 
    HvVcpu vcpus[HV_MAX_VCPUS];
    uint32_t cur_idx;
    HvVcpu *cur_vcpu;
} HvScheduler;

extern HvScheduler scheduler;
extern bool hv_scheduler_verbose;

void hv_scheduler_init(HvScheduler *scheduler);
HvVcpu *hv_scheduler_get_current(HvScheduler *scheduler);
bool hv_scheduler_is_idle_idx(uint32_t idx);
bool hv_scheduler_is_idle_vcpu(HvVcpu *vcpu);
bool hv_scheduler_has_runnable(HvScheduler *scheduler);
int hv_scheduler_pick_next_idx(HvScheduler *scheduler);
HvVcpu *hv_scheduler_pick_next(HvScheduler *scheduler);

// switch to a next vCPU
HypExceptAction hv_scheduler_switch_to(HvScheduler *scheduler, HypExceptState *hyp_state, int next_idx);
// HypExceptState is the stack frame 
HypExceptAction hv_scheduler_yield(HvScheduler *scheduler, HypExceptState *hyp_state);
// handles all the state transitions
HypExceptAction hv_scheduler_advance(HvScheduler *scheduler, HypExceptState *hyp_state);
HypExceptAction hv_scheduler_handle_wfi(HvScheduler *scheduler, HypExceptState *hyp_state);

// enter dummy/synthetic vCPU when no real guest vCPU is runnable
HypExceptAction hv_scheduler_enter_idle(HvScheduler *scheduler, HypExceptState *hyp_state);
