#pragma once

#include <stdint.h>
#include <stdbool.h>

#define HV_MAX_VIRQS 32 // max 32 interrupt lines

#include "virq-abi.h"
#include "vcpu.h"

#define HV_VIRQ_ERR_INVALID_VCPU -1
#define HV_VIRQ_ERR_INVALID_VIRQ -2
#define HV_VIRQ_ERR_BUSY -3 // cannot 
#define HV_VIRQ_ERR_NO_PENDING -4 // no pending virqs
#define HV_VIRQ_ERR_NOT_ACTIVE -5 // no active virqs

typedef struct { 
    bool initialized;
} HvVirqController;

extern HvVirqController virq_controller;

void hv_virq_init(HvVirqController *controller);
bool hv_virq_is_pending(HvVcpu *vcpu, VIRQ_Source virq_source);
bool hv_virq_is_active(HvVcpu *vcpu, VIRQ_Source virq_source);

// Sync HCR.VI to pending virtual IRQ status - if no pending virqs, clear HCR.VI, vice versa
void hv_virq_sync(HvVcpu *vcpu);

// raise virtual virq to a vCPU/guest
int hv_virq_raise(HvVcpu *vcpu, VIRQ_Source virq_source);

// Pick one pending virtual IRQ for the guest to handle. Returns virtual IRQ ID, or failure code.
int hv_virq_claim(HvVcpu *vcpu);

// Called after guest finishes handling virtual IRQ. Clears corresponding active bit for vCPU
int hv_virq_complete(HvVcpu *vcpu, VIRQ_Source virq);
