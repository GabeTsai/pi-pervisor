#pragma once

#include <stdint.h>

#include "hv/stage2.h"

#define HV_VM_OK             0
#define HV_VM_ERR_INVAL      -1

typedef struct HvVm {
    uint32_t vmid;
    HvIpa ipa_base;
    uint64_t ipa_size;
    HvPa pa_base;
    uint64_t pa_size;

    HvStage2 stage2;
} HvVm;

int hv_vm_init(HvVm *vm, uint32_t vmid, HvIpa ipa_base, uint64_t ipa_size, HvPa pa_base, uint64_t pa_size);
