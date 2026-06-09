#pragma once

#include "hv/fault.h"
#include "hv/vcpu.h"

#define HV_MMIO_OK              0
#define HV_MMIO_ERR_UNHANDLED  -1

int hv_mmio_handle_fault(HvVcpu *vcpu, const HvStage2FaultInfo *fault);
