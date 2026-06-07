#pragma once

#include <stdint.h>
#include <stdbool.h>

#define HV_MAX_VCPUS 1
#define HV_MAX_VIRQS 32 // max 32 interrupt lines

#include "virq-abi.h"

#define HV_VIRQ_ERR_INVALID_VCPU -1
#define HV_VIRQ_ERR_INVALID_VIRQ -2
#define HV_VIRQ_ERR_BUSY -3 // cannot 
#define HV_VIRQ_ERR_NO_PENDING -4 // no pending virqs
#define HV_VIRQ_ERR_NOT_ACTIVE -5 // no active virqs

typedef struct { 
    uint32_t id;
    uint32_t virq_pending;
    uint32_t virq_active;
} HvVcpu;

typedef struct { 
    HvVcpu vcpus[HV_MAX_VCPUS];
    HvVcpu *current_vcpu;
} HvVirqController;

extern HvVirqController virq_controller;

void hv_virq_init(HvVirqController *controller);
bool hv_virq_is_pending(HvVirqController *controller, uint32_t vcpu_id, VIRQ_Source virq_source);
bool hv_virq_is_active(HvVirqController *controller, uint32_t vcpu_id, VIRQ_Source virq_source);

// Sync HCR.VI to pending virtual IRQ status - if no pending virqs, clear HCR.VI, vice versa
void hv_virq_sync(HvVirqController *controller);

// raise virtual virq to a vCPU/guest
int hv_virq_raise(HvVirqController *controller, uint32_t vcpu_id, VIRQ_Source virq_source);

// Pick one pending virtual IRQ for the guest to handle. Returns virtual IRQ ID, or failure code.
int hv_virq_claim(HvVirqController *controller, uint32_t vcpu_id);

// Called after guest finishes handling virtual IRQ. Clears corresponding active bit for vCPU
int hv_virq_complete(HvVirqController *controller, uint32_t vcpu_id, VIRQ_Source virq);

static inline int hv_virq_raise_current(HvVirqController *controller, VIRQ_Source virq_source) {
    return hv_virq_raise(controller, controller->current_vcpu->id, virq_source);
}

static inline int hv_virq_claim_current(HvVirqController *controller) {
    return hv_virq_claim(controller, controller->current_vcpu->id);
}

static inline int hv_virq_complete_current(HvVirqController *controller, VIRQ_Source virq) {
    return hv_virq_complete(controller, controller->current_vcpu->id, virq);
}