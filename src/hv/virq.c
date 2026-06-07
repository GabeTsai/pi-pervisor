#include "hv/virq.h"
#include "hyp-regs.h"
#include "virq-abi.h"

HvVirqController virq_controller;

static inline int validate(uint32_t vcpu_id, VIRQ_Source virq_source) { 
    if (vcpu_id >= HV_MAX_VCPUS) {
        return HV_VIRQ_ERR_INVALID_VCPU;
    }
    if (virq_source >= HV_MAX_VIRQS) {
        return HV_VIRQ_ERR_INVALID_VIRQ;
    }
    return 0;
}

void hv_virq_init(HvVirqController *controller) {
    for (int i = 0; i < HV_MAX_VCPUS; i++) {
        controller->vcpus[i].id = i;
        controller->vcpus[i].virq_pending = 0;
        controller->vcpus[i].virq_active = 0;
    }
    controller->current_vcpu = &controller->vcpus[0];
}

bool hv_virq_is_pending(HvVirqController *controller, uint32_t vcpu_id, VIRQ_Source virq_source) {
    return (controller->vcpus[vcpu_id].virq_pending & (1 << virq_source));
}

bool hv_virq_is_active(HvVirqController *controller, uint32_t vcpu_id, VIRQ_Source virq_source) {
    return (controller->vcpus[vcpu_id].virq_active & (1 << virq_source));
}

void hv_virq_sync(HvVirqController *controller) {
    // if no pending virqs, clear HCR.VI, vice versa
    if (controller->vcpus[controller->current_vcpu->id].virq_pending == 0) {
        clear_hcr_bit(HCR_VI);
    } else {
        set_hcr_bit(HCR_VI);
    }
}

int hv_virq_raise(HvVirqController *controller, uint32_t vcpu_id, VIRQ_Source virq_source) {
    int err = validate(vcpu_id, virq_source);
    // if the virq is already pending or active, return an error
    if (err != 0) {
        return err;
    } else if (hv_virq_is_pending(controller, vcpu_id, virq_source) || hv_virq_is_active(controller, vcpu_id, virq_source)) {
        return HV_VIRQ_ERR_BUSY;
    }
    controller->vcpus[vcpu_id].virq_pending |= 1 << virq_source;
    hv_virq_sync(controller);
    return 0;
}

// for now pick the first bit set in the pending bitmap
int hv_virq_claim(HvVirqController *controller, uint32_t vcpu_id) {
    int err = validate(vcpu_id, 0);
    if (err != 0) {
        return err;
    }

    uint32_t pending = controller->vcpus[vcpu_id].virq_pending;
    if (pending == 0) {
        return HV_VIRQ_ERR_NO_PENDING;
    }

    for (int i = 0; i < HV_MAX_VIRQS; i++) {
        uint32_t virq_mask = 1u << i;
        if (pending & virq_mask) {
            controller->vcpus[vcpu_id].virq_pending &= ~virq_mask;
            controller->vcpus[vcpu_id].virq_active |= virq_mask;
            hv_virq_sync(controller);
            return i;
        }
    }

    return HV_VIRQ_ERR_NO_PENDING;
}

int hv_virq_complete(HvVirqController *controller, uint32_t vcpu_id, VIRQ_Source virq) {
    int err = validate(vcpu_id, virq);
    if (err != 0) {
        return err;
    } else if (!hv_virq_is_active(controller, vcpu_id, virq)) {
        return HV_VIRQ_ERR_NOT_ACTIVE;
    }
    controller->vcpus[vcpu_id].virq_active &= ~(1 << virq);
    hv_virq_sync(controller);
    return 0;
}
