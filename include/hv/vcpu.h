#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "hyp-exceptions.h"
#include "hyp-regs.h"

struct HvVm;

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
    uint32_t vbar; // guest vector base address register
} HvVcpuContext;

// each guest vCPU thinks it has its own timer
typedef struct {
    bool enabled;
    uint64_t deadline;
    uint64_t period;
} HvVcpuTimer;

typedef struct { 
    uint32_t id;
    struct HvVm *vm;
    HvVcpuState state;
    HvVcpuContext context;
    uint32_t virq_pending;
    uint32_t virq_active;
    HvVcpuTimer timer;
} HvVcpu;

void hv_vcpu_init(HvVcpu *vcpu, uint32_t id, uint32_t entry_point, uint32_t stack_top, uint32_t cpsr);
__attribute__((noreturn))
void hv_vcpu_enter_initial(HvVcpu *vcpu);
void hv_vcpu_block(HvVcpu *vcpu);
void hv_vcpu_wake(HvVcpu *vcpu);
void hv_vcpu_timer_start_periodic(HvVcpu *vcpu, uint64_t now, uint64_t period);
void hv_vcpu_timer_disable(HvVcpu *vcpu);
bool hv_vcpu_timer_expired(HvVcpu *vcpu, uint64_t now);
void hv_vcpu_timer_advance(HvVcpu *vcpu, uint64_t now);
void hv_vcpu_timer_delay(HvVcpu *vcpu, uint64_t now, uint64_t delay_ticks);
uint32_t hv_vcpu_timer_get_frequency(HvVcpu *vcpu);
void hv_vcpu_save(HvVcpu *vcpu, HypExceptState *hyp_state);
void hv_vcpu_load(HvVcpu *vcpu, HypExceptState *hyp_state);
