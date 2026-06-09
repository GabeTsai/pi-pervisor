#pragma once

#include "hv/vcpu.h"

// Hyp programs these controls for lower-mode guest stage-2 translation.
// They do not translate Hyp/EL2's own memory accesses.
void hv_mmu_activate_vcpu(const HvVcpu *vcpu);
void hv_mmu_deactivate_stage2(void);
