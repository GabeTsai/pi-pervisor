#include <stdbool.h>

#include "hv/mmio.h"
#include "hv/vcpu.h"
#include "hv/virtual-device.h"
#include "hv/vm.h"
#include "hyp-regs.h"
#include "panic.h"

typedef struct {
    uint32_t size;
    uint32_t reg;
    uint32_t value;
} HvMmioAccess;

// takes ISS bits of HSR and returns size of the access
static uint32_t mmio_access_size(uint32_t iss) {
    uint32_t sas = (iss >> HSR_ISS_ABORT_SAS_SHIFT) & HSR_ISS_ABORT_SAS_MASK;

    switch (sas) {
        case 0:
            return 1;
        case 1:
            return 2;
        case 2:
            return 4;
        default:
            return 0;
    }
}

static uint32_t mmio_access_reg(uint32_t iss) {
    return (iss >> HSR_ISS_ABORT_SRT_SHIFT) & HSR_ISS_ABORT_SRT_MASK;
}

// reads saved guest register from exception frame
static int mmio_read_guest_reg(const HypExceptState *hyp_state,
                               uint32_t reg,
                               uint32_t *value) {
    if (hyp_state == 0 || value == 0) {
        return HV_MMIO_ERR_UNHANDLED;
    }

    if (reg < 13) {
        *value = hyp_state->r[reg];
        return HV_MMIO_OK;
    }

    if (reg == 14) {
        *value = hyp_state->lr;
        return HV_MMIO_OK;
    }

    return HV_MMIO_ERR_UNHANDLED;
}

// write to guest register
static int mmio_write_guest_reg(HypExceptState *hyp_state,
                                uint32_t reg,
                                uint32_t value) {
    if (hyp_state == 0) {
        return HV_MMIO_ERR_UNHANDLED;
    }

    if (reg < 13) {
        hyp_state->r[reg] = value;
        return HV_MMIO_OK;
    }

    if (reg == 14) {
        hyp_state->lr = value;
        return HV_MMIO_OK;
    }

    return HV_MMIO_ERR_UNHANDLED;
}

// byte and halfword sign extend
// size is in bytes!
static uint32_t mmio_mask_value(uint32_t value, uint32_t size, bool sign_extend) {
    switch (size) {
        case 1:
            value &= 0xffu;
            if (sign_extend && (value & 0x80u) != 0) {
                value |= 0xffffff00u;
            }
            return value;
        case 2:
            value &= 0xffffu;
            if (sign_extend && (value & 0x8000u) != 0) {
                value |= 0xffff0000u;
            }
            return value;
        case 4:
            return value;
        default:
            return 0;
    }
}

// validate that fault is valid MMIO access
static int mmio_decode_access(HypExceptState *hyp_state,
                              const HvStage2FaultInfo *fault,
                              HvMmioAccess *access) {
    // reject null pointers, instructions, invalid offset/syndrome, stage 1 page table walks
    if (hyp_state == 0 ||
        fault == 0 ||
        access == 0 ||
        fault->is_instruction ||
        !fault->ipa_offset_valid ||
        (fault->iss & HSR_ISS_ABORT_ISV) == 0 ||
        (fault->iss & HSR_ISS_ABORT_S1PTW) != 0) {
        return HV_MMIO_ERR_UNHANDLED;
    }

    access->size = mmio_access_size(fault->iss);
    if (access->size == 0) {
        return HV_MMIO_ERR_UNHANDLED;
    }

    access->reg = mmio_access_reg(fault->iss);
    access->value = 0;
    return HV_MMIO_OK;
}

int hv_mmio_handle_fault(HvVcpu *vcpu,
                         HypExceptState *hyp_state,
                         const HvStage2FaultInfo *fault) {
    HvMmioAccess access;
    uint32_t value = 0;
    const HvVmRegion *region;

    if (vcpu == 0 || vcpu->vm == 0 || hyp_state == 0 || fault == 0) {
        return HV_MMIO_ERR_UNHANDLED;
    }

    if (mmio_decode_access(hyp_state, fault, &access) != HV_MMIO_OK) {
        trace("unsupported mmio fault decode: vcpu=%d vm=%d ipa=%p iss=%p\n",
              fault->vcpu_id,
              fault->vm_id,
              (uint32_t)fault->ipa,
              fault->iss);
        return HV_MMIO_ERR_UNHANDLED;
    }

    region = hv_vm_find_region(vcpu->vm, fault->ipa);
    if (region == 0 || region->type != HV_VM_REGION_MMIO) {
        return HV_MMIO_ERR_UNHANDLED;
    }

    if (fault->is_write) {
        if (mmio_read_guest_reg(hyp_state, access.reg, &value) != HV_MMIO_OK) {
            return HV_MMIO_ERR_UNHANDLED;
        }
        value = mmio_mask_value(value, access.size, false);
    }

    if (hv_virtual_device_access(vcpu, region, fault, access.size, value, &access.value) != HV_MMIO_OK) {
        trace("unsupported mmio access: vcpu=%d vm=%d ipa=%p offset=%p access=%d size=%d\n",
              fault->vcpu_id,
              fault->vm_id,
              (uint32_t)fault->ipa,
              (uint32_t)(fault->ipa - region->ipa_base),
              fault->access,
              access.size);
        return HV_MMIO_ERR_UNHANDLED;
    }

    if (!fault->is_write) {
        bool sign_extend = (fault->iss & HSR_ISS_ABORT_SSE) != 0;
        value = mmio_mask_value(access.value, access.size, sign_extend);
        if (mmio_write_guest_reg(hyp_state, access.reg, value) != HV_MMIO_OK) {
            return HV_MMIO_ERR_UNHANDLED;
        }
    }

    return HV_MMIO_OK;
}
