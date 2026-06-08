#include "hv/vcpu.h"
#include "hyp-regs.h"

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
