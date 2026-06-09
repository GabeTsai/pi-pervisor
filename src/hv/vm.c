#include "hv/vm.h"
#include "hv/stage2.h"

static int hv_vm_aligned(uint64_t value) {
    return (value & (HV_STAGE2_PAGE_SIZE - 1u)) == 0;
}

static int hv_vm_fits_v1_32bit(uint64_t base, uint64_t size) {
    if (base > 0xffffffffull || size > 0x100000000ull) {
        return 0;
    }

    return base <= (0x100000000ull - size);
}

static int hv_vm_region_backed(const HvVmRegion *region) {
    return region->type == HV_VM_REGION_RAM;
}

static int hv_vm_region_valid(const HvVmRegion *region) {
    if (region == 0 ||
        region->size == 0 ||
        !hv_vm_aligned(region->ipa_base) ||
        !hv_vm_aligned(region->size)) {
        return 0;
    }

    if (!hv_vm_fits_v1_32bit(region->ipa_base, region->size)) {
        return 0;
    }

    if (hv_vm_region_backed(region)) {
        if (!hv_vm_aligned(region->pa_base) ||
            !hv_vm_fits_v1_32bit(region->pa_base, region->size)) {
            return 0;
        }
    }

    return region->type == HV_VM_REGION_RAM ||
           region->type == HV_VM_REGION_RAM_GUARD ||
           region->type == HV_VM_REGION_MMIO;
}

static int hv_vm_region_contains(const HvVmRegion *region, HvIpa ipa) {
    if (region == 0) {
        return 0;
    }

    return ipa >= region->ipa_base &&
           (ipa - region->ipa_base) < region->size;
}

static int hv_vm_regions_overlap(const HvVmRegion *a, const HvVmRegion *b) {
    uint64_t a_end = a->ipa_base + a->size;
    uint64_t b_end = b->ipa_base + b->size;

    return a->ipa_base < b_end && b->ipa_base < a_end;
}

int hv_vm_init(HvVm *vm,
               uint32_t id,
               uint32_t vmid) {
    uint32_t *field = (uint32_t *)vm;

    if (vm == 0 ||
        id >= HV_MAX_VMS) {
        return HV_VM_ERR_INVAL;
    }

    for (uint32_t i = 0; i < sizeof(*vm) / sizeof(uint32_t); i++) {
        field[i] = 0;
    }

    vm->id = id;
    vm->vmid = vmid;

    return hv_stage2_init(vm);
}

int hv_vm_add_region(HvVm *vm, const HvVmRegion *region) {
    if (vm == 0 || !hv_vm_region_valid(region)) {
        return HV_VM_ERR_INVAL;
    }

    if (vm->region_count >= HV_MAX_VM_REGIONS) {
        return HV_VM_ERR_FULL;
    }

    for (uint32_t i = 0; i < vm->region_count; i++) {
        if (hv_vm_regions_overlap(&vm->regions[i], region)) {
            return HV_VM_ERR_OVERLAP;
        }
    }

    vm->regions[vm->region_count] = *region;
    vm->region_count++;
    return HV_VM_OK;
}

const HvVmRegion *hv_vm_find_region(const HvVm *vm, HvIpa ipa) {
    if (vm == 0) {
        return 0;
    }

    for (uint32_t i = 0; i < vm->region_count; i++) {
        if (hv_vm_region_contains(&vm->regions[i], ipa)) {
            return &vm->regions[i];
        }
    }

    return 0;
}

int hv_vm_build_stage2(HvVm *vm) {
    if (vm == 0) {
        return HV_VM_ERR_INVAL;
    }

    for (uint32_t i = 0; i < vm->region_count; i++) {
        HvVmRegion *region = &vm->regions[i];

        if (region->type != HV_VM_REGION_RAM) {
            continue;
        }

        int res = hv_stage2_map_region(vm,
                                       region->ipa_base,
                                       region->pa_base,
                                       region->size,
                                       region->attrs);
        if (res != HV_STAGE2_OK) {
            return res;
        }
    }

    return HV_VM_OK;
}
