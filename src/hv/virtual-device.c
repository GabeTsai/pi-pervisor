#include "hv/virtual-device.h"

#include "hv/mmio.h"
#include "hv/vuart.h"

int hv_virtual_device_access(HvVcpu *vcpu,
                             const HvVmRegion *region,
                             const HvStage2FaultInfo *fault,
                             uint32_t size,
                             uint32_t write_value,
                             uint32_t *read_value) {
    uint32_t offset;

    if (vcpu == 0 ||
        vcpu->vm == 0 ||
        region == 0 ||
        fault == 0 ||
        vcpu->vm->id >= HV_MAX_VMS) {
        return HV_MMIO_ERR_UNHANDLED;
    }

    offset = (uint32_t)(fault->ipa - region->ipa_base);

    if ((offset & (size - 1u)) != 0) {
        return HV_MMIO_ERR_UNHANDLED;
    }

    switch (region->device) {
        case HV_VM_MMIO_DEVICE_VUART:
            if (fault->is_write) {
                return hv_vuart_write(vcpu->vm->id, offset, write_value, size);
            }
            return hv_vuart_read(vcpu->vm->id, offset, size, read_value);
        default:
            return HV_MMIO_ERR_UNHANDLED;
    }
}
