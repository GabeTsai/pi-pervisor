#include "hv/vcpu.h"
#include "hv/mmu.h"
#include "hyp-enter-vcpu.h"
#include "hyp-regs.h"

#define HV_TIMER_TICKS_PER_SEC 1000000u

void hv_vcpu_init(HvVcpu *vcpu, uint32_t id, uint32_t entry_point, uint32_t stack_top, uint32_t cpsr) {
    struct HvVm *vm = vcpu->vm;
    uint32_t *field = (uint32_t *)vcpu;

    for (uint32_t i = 0; i < sizeof(*vcpu) / sizeof(uint32_t); i++) {
        field[i] = 0;
    }

    vcpu->id = id;
    vcpu->vm = vm;
    vcpu->state = HV_VCPU_RUNNABLE;
    vcpu->virq_pending = 0;
    vcpu->virq_active = 0;

    vcpu->context.hyp_state.elr_hyp = entry_point;
    vcpu->context.hyp_state.spsr_hyp = cpsr;
    vcpu->context.hyp_state.r[0] = stack_top;
    vcpu->context.banked_regs.sp_usr = stack_top;
    vcpu->context.banked_regs.sp_svc = stack_top;
}

void hv_vcpu_enter_initial(HvVcpu *vcpu) {
    hv_mmu_activate_vcpu(vcpu);
    hyp_enter_vcpu_context(&vcpu->context.hyp_state, &vcpu->context.banked_regs);
}

void hv_vcpu_block(HvVcpu *vcpu) {
    if (vcpu != 0) {
        vcpu->state = HV_VCPU_BLOCKED;
    }
}

void hv_vcpu_wake(HvVcpu *vcpu) {
    if (vcpu != 0 && vcpu->state == HV_VCPU_BLOCKED) {
        vcpu->state = HV_VCPU_RUNNABLE;
    }
}

// initialize periodic virtual timer for vCPU
void hv_vcpu_timer_start_periodic(HvVcpu *vcpu, uint64_t now, uint64_t period) {
    if (vcpu == 0 || period == 0) {
        return;
    }

    vcpu->timer.enabled = true;
    vcpu->timer.period = period;
    vcpu->timer.deadline = now + period;
}

// disable virtual timer for vCPU
void hv_vcpu_timer_disable(HvVcpu *vcpu) {
    if (vcpu == 0) {
        return;
    }

    vcpu->timer.enabled = false;
    vcpu->timer.period = 0;
    vcpu->timer.deadline = 0;
}

// check if virtual timer countdown expired
bool hv_vcpu_timer_expired(HvVcpu *vcpu, uint64_t now) {
    return vcpu != 0 && vcpu->timer.enabled && now >= vcpu->timer.deadline;
}

void hv_vcpu_timer_advance(HvVcpu *vcpu, uint64_t now) {
    if (vcpu == 0 || !vcpu->timer.enabled) {
        return;
    }

    if (vcpu->timer.period == 0) {
        hv_vcpu_timer_disable(vcpu);
        return;
    }
    // advance timer deadline to next expiration
    // NOTE: under heavy load, guest may miss multiple timer interrupts
    do {
        vcpu->timer.deadline += vcpu->timer.period;
    } while (now >= vcpu->timer.deadline);
}

void hv_vcpu_timer_delay(HvVcpu *vcpu, uint64_t now, uint64_t delay_ticks) {
    if (vcpu == 0 || delay_ticks == 0) {
        return;
    }

    vcpu->timer.enabled = true;
    vcpu->timer.deadline = now + delay_ticks;

    hv_vcpu_block(vcpu);
}

uint32_t hv_vcpu_timer_get_frequency(HvVcpu *vcpu) {
    if (vcpu == 0 || !vcpu->timer.enabled || vcpu->timer.period == 0) {
        return 0;
    }

    return HV_TIMER_TICKS_PER_SEC / (uint32_t)vcpu->timer.period;
}

static void copy_hyp_state(HypExceptState *dst, const HypExceptState *src) {
    uint32_t *dst_words = (uint32_t *)dst;
    const uint32_t *src_words = (const uint32_t *)src;

    for (uint32_t i = 0; i < sizeof(*dst) / sizeof(uint32_t); i++) {
        dst_words[i] = src_words[i];
    }
}

// vcpu context switching
// on save, save banked registers and hyp state
void hv_vcpu_save(HvVcpu *vcpu, HypExceptState *hyp_state) {
    copy_hyp_state(&vcpu->context.hyp_state, hyp_state);
    hyp_save_banked_regs(&vcpu->context.banked_regs);
    vcpu->context.vbar = read_vbar();
}

// on load, load banked registers and hyp state
void hv_vcpu_load(HvVcpu *vcpu, HypExceptState *hyp_state) {
    hyp_load_banked_regs(&vcpu->context.banked_regs);
    write_vbar(vcpu->context.vbar);
    copy_hyp_state(hyp_state, &vcpu->context.hyp_state);
}
