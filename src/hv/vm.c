#include "hv/vm.h"

static int hv_vm_aligned(uint64_t value) {
    return (value & (HV_STAGE2_PAGE_SIZE - 1u)) == 0;
}

static int hv_vm_fits_v1_32bit(uint64_t base, uint64_t size) {
    if (base > 0xffffffffull || size > 0x100000000ull) {
        return 0;
    }

    return base <= (0x100000000ull - size);
}

int hv_vm_init(HvVm *vm, uint32_t vmid, HvIpa ipa_base, uint64_t ipa_size, HvPa pa_base, uint64_t pa_size) {
    if (vm == 0 ||
        ipa_size == 0 ||
        pa_size == 0 ||
        !hv_vm_aligned(ipa_base) ||
        !hv_vm_aligned(ipa_size) ||
        !hv_vm_aligned(pa_base) ||
        !hv_vm_aligned(pa_size)) {
        return HV_VM_ERR_INVAL;
    }

    if (!hv_vm_fits_v1_32bit(ipa_base, ipa_size) ||
        !hv_vm_fits_v1_32bit(pa_base, pa_size)) {
        return HV_VM_ERR_INVAL;
    }

    vm->vmid = vmid;
    vm->ipa_base = ipa_base;
    vm->ipa_size = ipa_size;
    vm->pa_base = pa_base;
    vm->pa_size = pa_size;

    return hv_stage2_init(vm);
}
