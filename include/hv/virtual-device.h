#pragma once

#include <stdint.h>

#include "hv/fault.h"
#include "hv/vcpu.h"
#include "hv/vm.h"

int hv_virtual_device_access(HvVcpu *vcpu,
                             const HvVmRegion *region,
                             const HvStage2FaultInfo *fault,
                             uint32_t size,
                             uint32_t write_value,
                             uint32_t *read_value);
