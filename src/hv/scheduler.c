#include "hv/scheduler.h"
#include "check.h"
#include "hv/guest-image.h"
#include "hv/hypercall.h"
#include "hv/mmio.h"
#include "hv/mmu.h"
#include "hv/virq.h"
#include "hyp-enter-lower.h"
#include "hyp-regs.h"
#include "panic.h"
#include "timer.h"

HvScheduler scheduler = {0};
bool hv_scheduler_verbose = false;

extern void hv_idle_vcpu_entry(void);

static uint32_t idle_stack[128] __attribute__((aligned(8)));

static void hv_scheduler_sync_traps(uint32_t next_idx) {
    if (hv_scheduler_is_idle_idx(next_idx)) {
        clear_hcr_bit(HCR_TWI);
    } else {
        set_hcr_bit(HCR_TWI);
    }
}

// we're not doing thumb state in this repo for now but good for completeness
static void hv_scheduler_advance_trapped_wfi(HypExceptState *hyp_state) {
    uint32_t instr_len = HSR_IL(hyp_state->hsr) != 0 ? 4 : 2;
    hyp_state->elr_hyp += instr_len;
}

void hv_scheduler_init(HvScheduler *scheduler) {
    scheduler->cur_idx = 0;

    for (uint32_t i = 0; i < HV_MAX_VCPUS; i++) {
        // init vCPU scheduler params to idle
        scheduler->vcpus[i].id = i;
        scheduler->vcpus[i].vm = 0;
        scheduler->vcpus[i].state = HV_VCPU_IDLE;
        scheduler->vcpus[i].virq_pending = 0;
        scheduler->vcpus[i].virq_active = 0;
    }

    for (uint32_t i = 0; i < HV_MAX_GUEST_VCPUS; i++) {
        int vm_res = hv_vm_init(&scheduler->vm[i],
                                i,
                                i);
        assert(vm_res == HV_VM_OK, "failed to initialize guest VM");

        HvVmRegion ram_region = {
            .ipa_base = GUEST_BASE,
            .size = GUEST_SLOT_SIZE - HV_STAGE2_PAGE_SIZE,
            .pa_base = GUEST_SLOT_BASE(i),
            .attrs = HV_STAGE2_PAGE_NORMAL_NC_RW,
            .type = HV_VM_REGION_RAM,
        };
        vm_res = hv_vm_add_region(&scheduler->vm[i], &ram_region);
        assert(vm_res == HV_VM_OK, "failed to add guest RAM region");

        HvVmRegion guard_region = {
            .ipa_base = GUEST_BASE + GUEST_SLOT_SIZE - HV_STAGE2_PAGE_SIZE,
            .size = HV_STAGE2_PAGE_SIZE,
            .pa_base = 0,
            .attrs = 0,
            .type = HV_VM_REGION_RAM_GUARD,
        };
        vm_res = hv_vm_add_region(&scheduler->vm[i], &guard_region);
        assert(vm_res == HV_VM_OK, "failed to add guest guard region");

        HvVmRegion uart_region = {
            .ipa_base = HV_GUEST_UART_BASE,
            .size = HV_GUEST_UART_SIZE,
            .pa_base = 0,
            .attrs = 0,
            .type = HV_VM_REGION_MMIO,
            .device = HV_VM_MMIO_DEVICE_VUART,
        };
        vm_res = hv_vm_add_region(&scheduler->vm[i], &uart_region);
        assert(vm_res == HV_VM_OK, "failed to add guest UART MMIO region");

        vm_res = hv_vm_build_stage2(&scheduler->vm[i]);
        assert(vm_res == HV_VM_OK, "failed to build guest stage-2 tables");

        if (hv_scheduler_verbose) {
            trace("vm map id=%d vmid=%d ipa=%p pa=%p root=%p\n",
                  scheduler->vm[i].id,
                  scheduler->vm[i].vmid,
                  (uint32_t)ram_region.ipa_base,
                  (uint32_t)ram_region.pa_base,
                  (uint32_t)scheduler->vm[i].stage2.root_pa);
        }
        
        scheduler->vcpus[i].vm = &scheduler->vm[i];
    }

    hv_vcpu_init(&scheduler->vcpus[HV_IDLE_VCPU_IDX],
                 HV_IDLE_VCPU_ID,
                 (uint32_t)hv_idle_vcpu_entry,
                 (uint32_t)&idle_stack[sizeof(idle_stack) / sizeof(idle_stack[0])],
                 HYP_LOWER_SVC_CPSR);
    scheduler->vcpus[HV_IDLE_VCPU_IDX].state = HV_VCPU_IDLE;

    scheduler->vcpus[0].state = HV_VCPU_RUNNABLE;
    scheduler->cur_vcpu = &scheduler->vcpus[0];

    set_hcr_bit(HCR_TWI);
}

HvVcpu *hv_scheduler_get_current(HvScheduler *scheduler) {
    return scheduler->cur_vcpu;
}

bool hv_scheduler_is_idle_idx(uint32_t idx) {
    return idx == HV_IDLE_VCPU_IDX;
}

bool hv_scheduler_is_idle_vcpu(HvVcpu *vcpu) {
    return vcpu != 0 && vcpu->id == HV_IDLE_VCPU_ID;
}

bool hv_scheduler_has_runnable(HvScheduler *scheduler) {
    for (uint32_t i = 0; i < HV_MAX_GUEST_VCPUS; i++) {
        if (scheduler->vcpus[i].state == HV_VCPU_RUNNABLE) {
            return true;
        }
    }

    return false;
}

// simple round-robin scheduler. Note that it is possible for it to return the current vCPU
int hv_scheduler_pick_next_idx(HvScheduler *scheduler) {
    uint32_t start = scheduler->cur_idx;

    for (uint32_t offset = 1; offset <= HV_MAX_VCPUS; offset++) {
        uint32_t idx = (start + offset) % HV_MAX_VCPUS;

        if (hv_scheduler_is_idle_idx(idx)) {
            continue;
        }

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
            if (hv_scheduler_is_idle_vcpu(prev)) {
                prev->state = HV_VCPU_IDLE;
            } else {
                prev->state = HV_VCPU_RUNNABLE;
            }
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

    hv_scheduler_sync_traps(next_idx);
    hv_mmu_activate_vcpu(next);
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
        return hv_scheduler_enter_idle(scheduler, hyp_state);
    }

    if (cur->state == HV_VCPU_RUNNING) {
        if (hv_scheduler_is_idle_vcpu(cur)) {
            cur->state = HV_VCPU_IDLE;
        } else {
            cur->state = HV_VCPU_RUNNABLE;
        }
    }

    int next_idx = hv_scheduler_pick_next_idx(scheduler);
    if (next_idx < 0) { 
        return hv_scheduler_enter_idle(scheduler, hyp_state);
    }
    return hv_scheduler_switch_to(scheduler, hyp_state, next_idx);
}

HypExceptAction hv_scheduler_handle_wfi(HvScheduler *scheduler, HypExceptState *hyp_state) {
    HvVcpu *cur = scheduler->cur_vcpu;
    if (cur == 0 || hv_scheduler_is_idle_vcpu(cur)) {
        return hv_scheduler_enter_idle(scheduler, hyp_state);
    }

    hv_scheduler_advance_trapped_wfi(hyp_state);

    if (cur->virq_pending != 0) {
        hv_virq_sync(cur);
        return HYP_ACTION_RETURN;
    }

    hv_vcpu_block(cur);
    return hv_scheduler_advance(scheduler, hyp_state);
}

HypExceptAction hv_scheduler_enter_idle(HvScheduler *scheduler, HypExceptState *hyp_state) {
    if (scheduler->cur_vcpu == &scheduler->vcpus[HV_IDLE_VCPU_IDX]) {
        scheduler->vcpus[HV_IDLE_VCPU_IDX].state = HV_VCPU_RUNNING;
        scheduler->cur_idx = HV_IDLE_VCPU_IDX;
        hv_scheduler_sync_traps(HV_IDLE_VCPU_IDX);
        hv_mmu_activate_vcpu(&scheduler->vcpus[HV_IDLE_VCPU_IDX]);
        hv_virq_sync(&scheduler->vcpus[HV_IDLE_VCPU_IDX]);
        return HYP_ACTION_RETURN;
    }

    return hv_scheduler_switch_to(scheduler, hyp_state, HV_IDLE_VCPU_IDX);
}

void hv_vtimer_rearm_physical(HvScheduler *scheduler) {
    bool found = false;
    uint64_t earliest = 0;

    for (uint32_t i = 0; i < HV_MAX_GUEST_VCPUS; i++) {
        HvVcpu *vcpu = &scheduler->vcpus[i];

        if (vcpu->state == HV_VCPU_IDLE ||
            vcpu->state == HV_VCPU_EXITED ||
            !vcpu->timer.enabled) {
            continue;
        }

        if (!found || vcpu->timer.deadline < earliest) {
            earliest = vcpu->timer.deadline;
            found = true;
        }
    }

    if (!found) {
        TIM_Disable_IRQ();
        return;
    }

    uint64_t now = TIM_SYS_Get_Ticks();
    uint64_t delta = earliest > now ? earliest - now : 1;

    if (delta > TIM_ARM_TIMER_MAX_DELTA_US) {
        delta = TIM_ARM_TIMER_MAX_DELTA_US;
    }

    TIM_Arm_Usec((uint32_t)delta);
}
