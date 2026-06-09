#include "hv/mmio.h"
#include "hv/vcpu.h"
#include "panic.h"

int hv_mmio_handle_fault(HvVcpu *vcpu, const HvStage2FaultInfo *fault) {
    if (vcpu == 0 || fault == 0) {
        return HV_MMIO_ERR_UNHANDLED;
    }

    trace("unsupported mmio fault: vcpu=%d vm=%d ipa=%p access=%d\n",
          fault->vcpu_id,
          fault->vm_id,
          (uint32_t)fault->ipa,
          fault->access);
    return HV_MMIO_ERR_UNHANDLED;
}
