#include "hv/virq.h"
#include "hyp-regs.h"
#include "hv/virq-abi.h"

HvVirqController virq_controller;

static inline int validate(HvVcpu *vcpu, VIRQ_Source virq_source) { 
    if (vcpu == 0) {
        return HV_VIRQ_ERR_INVALID_VCPU;
    }
    if (virq_source >= HV_MAX_VIRQS) {
        return HV_VIRQ_ERR_INVALID_VIRQ;
    }
    return 0;
}

void hv_virq_init(HvVirqController *controller) {
    controller->initialized = true;
}

bool hv_virq_is_pending(HvVcpu *vcpu, VIRQ_Source virq_source) {
    if (validate(vcpu, virq_source) != 0) {
        return false;
    }

    return (vcpu->virq_pending & (1 << virq_source));
}

bool hv_virq_is_active(HvVcpu *vcpu, VIRQ_Source virq_source) {
    if (validate(vcpu, virq_source) != 0) {
        return false;
    }

    return (vcpu->virq_active & (1 << virq_source));
}

void hv_virq_sync(HvVcpu *vcpu) {
    // if no pending virqs, clear HCR.VI, vice versa
    if (vcpu == 0 || vcpu->virq_pending == 0) {
        clear_hcr_bit(HCR_VI);
    } else {
        set_hcr_bit(HCR_VI);
    }
}

int hv_virq_raise(HvVcpu *vcpu, VIRQ_Source virq_source) {
    int err = validate(vcpu, virq_source);
    // if the virq is already pending or active, return an error
    if (err != 0) {
        return err;
    } else if (hv_virq_is_pending(vcpu, virq_source) || hv_virq_is_active(vcpu, virq_source)) {
        return HV_VIRQ_ERR_BUSY;
    }
    vcpu->virq_pending |= 1 << virq_source;
    return 0;
}

// for now pick the first bit set in the pending bitmap
int hv_virq_claim(HvVcpu *vcpu) {
    int err = validate(vcpu, 0);
    if (err != 0) {
        return err;
    }

    uint32_t pending = vcpu->virq_pending;
    if (pending == 0) {
        return HV_VIRQ_ERR_NO_PENDING;
    }

    for (int i = 0; i < HV_MAX_VIRQS; i++) {
        uint32_t virq_mask = 1u << i;
        if (pending & virq_mask) {
            vcpu->virq_pending &= ~virq_mask;
            vcpu->virq_active |= virq_mask;
            return i;
        }
    }

    return HV_VIRQ_ERR_NO_PENDING;
}

int hv_virq_complete(HvVcpu *vcpu, VIRQ_Source virq) {
    int err = validate(vcpu, virq);
    if (err != 0) {
        return err;
    } else if (!hv_virq_is_active(vcpu, virq)) {
        return HV_VIRQ_ERR_NOT_ACTIVE;
    }
    vcpu->virq_active &= ~(1 << virq);
    return 0;
}
