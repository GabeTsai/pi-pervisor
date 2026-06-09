#include <stdint.h>

#include "aarch32.h"
#include "hv/mmu.h"
#include "hv/vm.h"
#include "hyp-regs.h"

static uint64_t hv_mmu_vttbr_for_vm(const HvVm *vm) {
    uint64_t vmid = ((uint64_t)vm->vmid & VTTBR_VMID_MASK) << VTTBR_VMID_SHIFT;
    uint64_t baddr = (uint64_t)vm->stage2.root_pa & VTTBR_BADDR_MASK;
    return vmid | baddr;
}

void hv_mmu_deactivate_stage2(void) {
    clear_hcr_bit(HCR_VM);
    hyp_invalidate_stage2_tlb_all();
    DSB();
    ISB();
}

void hv_mmu_activate_vcpu(const HvVcpu *vcpu) {
    if (vcpu == 0 || vcpu->vm == 0) {
        hv_mmu_deactivate_stage2();
        return;
    }

    // Implementation based off of G4-4636 
    // G4-4635 - TLB maintenance instruction only guaranteed to be complete after execution of DBB
    DSB(); 
    // G4.9.2 - Any writes to control bits that change TLB behavior must be followed by ISB and invalidate
    write_vtcr(HV_STAGE2_VTCR_DEFAULT);
    write_vttbr(hv_mmu_vttbr_for_vm(vcpu->vm));
    ISB(); 
    hyp_invalidate_stage2_tlb_all(); 
    // G4-4636 - write to translation tables must be followed by DSB and ISB
    DSB(); 
    ISB();
    set_hcr_bit(HCR_VM);
    ISB();
}
