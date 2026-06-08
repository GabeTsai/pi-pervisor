#include "hv/vcpu.h"
#include "hyp-enter-vcpu.h"
#include "hyp-regs.h"

void hv_vcpu_init(HvVcpu *vcpu, uint32_t id, uint32_t entry_point, uint32_t stack_top, uint32_t cpsr) {
    uint32_t *field = (uint32_t *)vcpu;

    for (uint32_t i = 0; i < sizeof(*vcpu) / sizeof(uint32_t); i++) {
        field[i] = 0;
    }

    vcpu->id = id;
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
    hyp_enter_vcpu_context(&vcpu->context.hyp_state, &vcpu->context.banked_regs);
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
}

// on load, load banked registers and hyp state
void hv_vcpu_load(HvVcpu *vcpu, HypExceptState *hyp_state) {
    hyp_load_banked_regs(&vcpu->context.banked_regs);
    copy_hyp_state(hyp_state, &vcpu->context.hyp_state);
}
