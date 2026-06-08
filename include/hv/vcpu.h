#pragma once

#include <stdint.h>
#include "hyp-exceptions.h"
#include "hyp-regs.h"

typedef enum { 
    HV_VCPU_IDLE, // not running, not ready to run
    HV_VCPU_RUNNABLE, // not running, ready to run
    HV_VCPU_RUNNING, 
    HV_VCPU_BLOCKED, // waiting to be woken up
    HV_VCPU_EXITED, // exited, but can potentially run again
} HvVcpuState;

// per-vCPU guest execution context
// contains saved CPU state needed to resume guest vCPU
typedef struct { 
    HypExceptState hyp_state; // saved return frame 
    HypBankedRegs banked_regs; // saved banked lower-mode registers
} HvVcpuContext;

typedef struct { 
    uint32_t id;
    HvVcpuState state;
    HvVcpuContext context;
    uint32_t virq_pending;
    uint32_t virq_active;

} HvVcpu;

int hv_vcpu_init(HvVcpu *vcpu, uint32_t id, uint32_t entry_point, uint32_t stack_top, uint32_t cpsr);
void hv_vcpu_save(HvVcpu *vcpu, HypExceptState *hyp_state);
void hv_vcpu_load(HvVcpu *vcpu, HypExceptState *hyp_state);