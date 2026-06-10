#pragma once

#include <stdint.h>

#include "hv/stage2.h"

#define HV_VM_OK             0
#define HV_VM_ERR_INVAL      -1
#define HV_VM_ERR_RANGE      -2
#define HV_VM_ERR_FULL       -3
#define HV_VM_ERR_OVERLAP    -4

#define HV_MAX_VMS           2
#define HV_MAX_VM_REGIONS    8

typedef enum {
    HV_VM_REGION_RAM, // known guest RAM, with a real PA
    HV_VM_REGION_RAM_GUARD, // known guest boundary/guard, not mapped
    HV_VM_REGION_MMIO, // known guest device/MMIO IPA range. should fault into MMIO dispatcher
} HvVmRegionType;

typedef enum {
    HV_VM_MMIO_DEVICE_NONE,
    HV_VM_MMIO_DEVICE_VUART,
} HvVmMmioDevice;

typedef struct {
    HvIpa ipa_base;
    uint64_t size;
    HvPa pa_base;
    uint64_t attrs;
    HvVmRegionType type;
    HvVmMmioDevice device;
} HvVmRegion;

// Hyp Virtual Memory Space. Responsible for the guest address-space and policy
typedef struct HvVm {
    uint32_t id; // id slot in the pool table 
    uint32_t vmid; // virtual memory space ID 
    HvVmRegion regions[HV_MAX_VM_REGIONS];
    uint32_t region_count;

    HvStage2 stage2;
} HvVm;

int hv_vm_init(HvVm *vm,
               uint32_t id,
               uint32_t vmid);
int hv_vm_add_region(HvVm *vm, const HvVmRegion *region);
const HvVmRegion *hv_vm_find_region(const HvVm *vm, HvIpa ipa);
int hv_vm_build_stage2(HvVm *vm);
