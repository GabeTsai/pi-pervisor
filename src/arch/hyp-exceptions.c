#include <stdint.h>
#include "hyp-exceptions.h"
#include "printk.h"
#include "hyp-regs.h"
#include "hv/hypercall.h"
#include "hv/mmio.h"
#include "hv/scheduler.h"
#include "hv/stage2.h"
#include "hv/virq.h"
#include "hv/vm.h"
#include "check.h" 
#include "panic.h"
#include "aarch32.h"
#include "generic_timer.h"
#include "timer.h"

bool hyp_except_verbose = false;
static volatile uint32_t counter = 0;

uint32_t hyp_get_irq_count(void) {
    return counter;
}

static const char *hsr_ec_name(uint32_t ec) {
    switch (ec) {
        case HSR_EC_UNKNOWN:
            return "unknown";
        case HSR_EC_WFI_WFE:
            return "wfi/wfe";
        case HSR_EC_CP15_MCR_MRC:
            return "cp15 mcr/mrc";
        case HSR_EC_CP15_MCRR_MRRC:
            return "cp15 mcrr/mrrc";
        case HSR_EC_CP14_MCR_MRC:
            return "cp14 mcr/mrc";
        case HSR_EC_LDC_STC:
            return "ldc/stc";
        case HSR_EC_FP_ASIMD:
            return "fp/asimd";
        case HSR_EC_VMRS_ID_TRAP:
            return "vmrs id trap";
        case HSR_EC_CP14_MRRC:
            return "cp14 mrrc";
        case HSR_EC_ILLEGAL_EXCEPTION_RETURN_A32:
            return "illegal exception return";
        case HSR_EC_SVC_A32:
            return "svc a32";
        case HSR_EC_HVC_A32:
            return "hvc a32";
        case HSR_EC_SMC_A32:
            return "smc a32";
        case HSR_EC_PREFETCH_ABORT_LOWER:
            return "prefetch abort lower";
        case HSR_EC_PREFETCH_ABORT_SAME:
            return "prefetch abort same";
        case HSR_EC_PC_ALIGNMENT_FAULT:
            return "pc alignment fault";
        case HSR_EC_DATA_ABORT_LOWER:
            return "data abort lower";
        case HSR_EC_DATA_ABORT_SAME:
            return "data abort same";
        default:
            return "reserved/unknown";
    }
}

void hyp_dump_exception_state(HypExceptState *hyp_state) { 
    trace("-------- HYP EXCEPTION STATE --------\n");
    trace("EXCEPTION TYPE: %d\n", hyp_state->exception_type);
    trace("HSR: %p\n", hyp_state->hsr);
    trace("HSR EC: %d (%s)\n", HSR_EC(hyp_state->hsr), hsr_ec_name(HSR_EC(hyp_state->hsr)));
    trace("HSR ISS: %p\n", HSR_ISS(hyp_state->hsr));
    trace("ELR_HYP: %p\n", hyp_state->elr_hyp);
    trace("SPSR_HYP: %b\n", hyp_state->spsr_hyp);
    trace("HDFAR: %p\n", hyp_state->hdfar);
    trace("HIFAR: %p\n", hyp_state->hifar);
    trace("HPFAR: %p\n", hyp_state->hpfar);
    trace("-------------------------------------\n\n");
}

// EC values are from 0 to 63 so we need 64-bit masks
static const uint64_t valid_hsr_ec[] = {
    [HYP_EXCEPTION_UNKNOWN] =
        EC_BIT(HSR_EC_UNKNOWN),

    [HYP_EXCEPTION_UNDEF_INSTR] =
        EC_BIT(HSR_EC_UNKNOWN),

    [HYP_EXCEPTION_HVC_FROM_HYP] =
        EC_BIT(HSR_EC_HVC_A32),

    [HYP_EXCEPTION_PREFETCH_ABORT] =
        EC_BIT(HSR_EC_PREFETCH_ABORT_LOWER) |
        EC_BIT(HSR_EC_PREFETCH_ABORT_SAME) |
        EC_BIT(HSR_EC_PC_ALIGNMENT_FAULT),

    [HYP_EXCEPTION_DATA_ABORT] =
        EC_BIT(HSR_EC_DATA_ABORT_LOWER) |
        EC_BIT(HSR_EC_DATA_ABORT_SAME),

    [HYP_EXCEPTION_LOWER_SYNC] =
        EC_BIT(HSR_EC_HVC_A32) |
        EC_BIT(HSR_EC_WFI_WFE) |
        EC_BIT(HSR_EC_PREFETCH_ABORT_LOWER) |
        EC_BIT(HSR_EC_DATA_ABORT_LOWER) |
        EC_BIT(HSR_EC_CP15_MCR_MRC) |
        EC_BIT(HSR_EC_CP15_MCRR_MRRC) |
        EC_BIT(HSR_EC_CP14_MCR_MRC) |
        EC_BIT(HSR_EC_CP14_MRRC) |
        EC_BIT(HSR_EC_FP_ASIMD),
};

bool hsr_ec_valid(uint32_t hsr, HypExceptType except_type) { 
    uint8_t hsr_ec = HSR_EC(hsr);
    if (except_type < 0 ||
        (uint32_t)except_type >= sizeof(valid_hsr_ec) / sizeof(valid_hsr_ec[0])) {
        return false;
    }
    return (valid_hsr_ec[except_type] & EC_BIT(hsr_ec)) != 0;
}

// helper function to signal timer interrupts to guests when their countdown/deadline expires
static bool deliver_expired_timers(uint64_t now) {
    bool delivered = false;

    for (uint32_t i = 0; i < HV_MAX_GUEST_VCPUS; i++) {
        HvVcpu *vcpu = &scheduler.vcpus[i];

        if (vcpu->state == HV_VCPU_IDLE ||
            vcpu->state == HV_VCPU_EXITED ||
            !hv_vcpu_timer_expired(vcpu, now)) {
            continue;
        }

        int res = hv_virq_raise(vcpu, VIRQ_TIMER);
        if (res == HV_VIRQ_ERR_BUSY) {
            hv_vcpu_timer_advance(vcpu, now);
            delivered = true;
            continue;
        }

        if (res != 0) {
            trace("Failed to raise timer VIRQ: %d\n", res);
            continue;
        }

        hv_vcpu_timer_advance(vcpu, now);
        delivered = true;
    }

    return delivered;
}

HypExceptAction hyp_handle_exception(HypExceptState *hyp_state) { 
    if (hyp_except_verbose) {
        hyp_dump_exception_state(hyp_state);
    }

    switch (hyp_state->exception_type) { 
        case HYP_EXCEPTION_UNDEF_INSTR:
            return handle_undef_instr(hyp_state);
        case HYP_EXCEPTION_UNKNOWN:
            return handle_unknown(hyp_state);
        case HYP_EXCEPTION_HVC_FROM_HYP:
            return handle_hvc_from_hyp(hyp_state);
        case HYP_EXCEPTION_PREFETCH_ABORT:
            return handle_prefetch_abort(hyp_state);
        case HYP_EXCEPTION_DATA_ABORT:
            return handle_data_abort(hyp_state);
        case HYP_EXCEPTION_LOWER_SYNC:
            return handle_lower_sync(hyp_state);
        case HYP_EXCEPTION_IRQ:
            return handle_irq(hyp_state);
        case HYP_EXCEPTION_FIQ:
            return handle_fiq(hyp_state);
        default:
            if (!hyp_except_verbose) {
                hyp_dump_exception_state(hyp_state);
            }
            trace("Unknown Hyp exception type\n");
            return HYP_ACTION_HALT;
    }
}

static void hyp_dump_unexpected_exception_state(HypExceptState *hyp_state) {
    if (!hyp_except_verbose) {
        hyp_dump_exception_state(hyp_state);
    }
}

HypExceptAction handle_hvc_from_hyp(HypExceptState *hyp_state) { 
    trace("HVC from Hyp\n");
    assert(hsr_ec_valid(hyp_state->hsr, HYP_EXCEPTION_HVC_FROM_HYP), "HSR EC mismatch");
    return HYP_ACTION_RETURN;
}

HypExceptAction handle_hvc_from_lower(HypExceptState *hyp_state) { 
    assert(HSR_EC(hyp_state->hsr) == HSR_EC_HVC_A32, "expected lower-mode HVC");
    assert(hsr_iss_imm16(hyp_state->hsr) == HYPERCALL_HVC_IMM, "bad HVC immediate");
    return hyp_handle_guest_hypercall(hyp_state);
}

static const char *stage2_access_name(HvStage2Access access) {
    switch (access) {
        case HV_STAGE2_ACCESS_READ:
            return "read";
        case HV_STAGE2_ACCESS_WRITE:
            return "write";
        case HV_STAGE2_ACCESS_INSTRUCTION:
            return "instruction";
        default:
            return "unknown";
    }
}

static void decode_stage2_fault(HvVcpu *vcpu,
                                HypExceptState *hyp_state,
                                HvStage2FaultInfo *fault) {
    uint32_t iss = HSR_ISS(hyp_state->hsr);
    uint32_t ec = HSR_EC(hyp_state->hsr);
    HvIpa fault_ipa = ((uint64_t)(hyp_state->hpfar & 0xfffffff0u)) << 8;
    bool is_instruction = ec == HSR_EC_PREFETCH_ABORT_LOWER;
    bool is_write = !is_instruction && ((iss & HSR_ISS_ABORT_WNR) != 0);
    bool offset_valid = !is_instruction && ((iss & HSR_ISS_ABORT_FNV) == 0);

    if (offset_valid) {
        fault_ipa |= hyp_state->hdfar & (HV_STAGE2_PAGE_SIZE - 1u);
    }

    fault->vcpu_id = vcpu == 0 ? 0xffffffffu : vcpu->id;
    fault->vm_id = vcpu == 0 || vcpu->vm == 0 ? 0xffffffffu : vcpu->vm->id;
    fault->ec = ec;
    fault->iss = iss;
    fault->status = iss & HSR_ISS_FAULT_STATUS_MASK;
    fault->level = fault->status & 0x3u;
    fault->ipa = fault_ipa;
    fault->is_write = is_write;
    fault->is_instruction = is_instruction;
    fault->ipa_offset_valid = offset_valid;
    fault->access = is_instruction ? HV_STAGE2_ACCESS_INSTRUCTION :
                    is_write ? HV_STAGE2_ACCESS_WRITE :
                    HV_STAGE2_ACCESS_READ;
}

static void log_stage2_fault(const HvStage2FaultInfo *fault, const char *route) {
    if (!hyp_except_verbose) {
        return;
    }

    trace("stage2 fault: route=%s vcpu=%d vm=%d ipa=%p access=%s level=%d status=%p iss=%p\n",
          route,
          fault->vcpu_id,
          fault->vm_id,
          (uint32_t)fault->ipa,
          stage2_access_name(fault->access),
          fault->level,
          fault->status,
          fault->iss);
}

static void advance_trapped_guest_instr(HypExceptState *hyp_state) {
    uint32_t instr_len = HSR_IL(hyp_state->hsr) != 0 ? 4 : 2;
    hyp_state->elr_hyp += instr_len;
}

static HypExceptAction stop_current_guest_vcpu(HvVcpu *vcpu, HypExceptState *hyp_state) {
    hv_vcpu_timer_disable(vcpu);
    vcpu->state = HV_VCPU_EXITED;
    hv_vtimer_rearm_physical(&scheduler);
    return hv_scheduler_advance(&scheduler, hyp_state);
}

HypExceptAction handle_guest_abort(HypExceptState *hyp_state) {
    HvVcpu *vcpu = hv_scheduler_get_current(&scheduler);

    if (hyp_except_verbose) {
        uint32_t iss = HSR_ISS(hyp_state->hsr);
        uint32_t status = iss & HSR_ISS_FAULT_STATUS_MASK;
        uint64_t fault_ipa = ((uint64_t)(hyp_state->hpfar & 0xfffffff0u)) << 8;

        trace("Guest abort: ec=%d (%s) iss=%p status=%p\n",
            HSR_EC(hyp_state->hsr),
            hsr_ec_name(HSR_EC(hyp_state->hsr)),
            iss,
            status);
        trace("Guest abort regs: elr=%p hdfar=%p hifar=%p hpfar=%p ipa_hi=%p ipa_lo=%p\n",
            hyp_state->elr_hyp,
            hyp_state->hdfar,
            hyp_state->hifar,
            hyp_state->hpfar,
            (uint32_t)(fault_ipa >> 32),
            (uint32_t)fault_ipa);
    }

    if (vcpu == 0 || hv_scheduler_is_idle_vcpu(vcpu)) {
        return HYP_ACTION_HALT;
    }

    HvStage2FaultInfo fault;
    decode_stage2_fault(vcpu, hyp_state, &fault);

    if (vcpu->vm == 0) {
        log_stage2_fault(&fault, "no-vm");
        return HYP_ACTION_HALT;
    }

    const HvVmRegion *region = hv_vm_find_region(vcpu->vm, fault.ipa);
    if (region == 0) {
        log_stage2_fault(&fault, "unexpected-unmapped");
        return stop_current_guest_vcpu(vcpu, hyp_state);
    }

    if (region->type == HV_VM_REGION_RAM_GUARD) {
        log_stage2_fault(&fault, "ram-guard-exit");
        return stop_current_guest_vcpu(vcpu, hyp_state);
    }

    if (region->type == HV_VM_REGION_MMIO) {
        log_stage2_fault(&fault, "mmio");
        if (hv_mmio_handle_fault(vcpu, hyp_state, &fault) == HV_MMIO_OK) {
            advance_trapped_guest_instr(hyp_state);
            return HYP_ACTION_RETURN;
        }
        log_stage2_fault(&fault, "mmio-unhandled");
        return stop_current_guest_vcpu(vcpu, hyp_state);
    }

    HvPa pa;
    if (hv_stage2_translate(vcpu->vm, fault.ipa, &pa) != HV_STAGE2_OK) {
        log_stage2_fault(&fault, "valid-ram-unmapped");
        return stop_current_guest_vcpu(vcpu, hyp_state);
    }

    log_stage2_fault(&fault, "unexpected-mapped-ram");
    return stop_current_guest_vcpu(vcpu, hyp_state);
}

HypExceptAction handle_undef_instr(HypExceptState *hyp_state) { 
    hyp_dump_unexpected_exception_state(hyp_state);
    trace("Undefined instruction in Hyp\n");
    assert(hsr_ec_valid(hyp_state->hsr, HYP_EXCEPTION_UNDEF_INSTR), "HSR EC mismatch");
    return HYP_ACTION_HALT;
}

HypExceptAction handle_unknown(HypExceptState *hyp_state) { 
    hyp_dump_unexpected_exception_state(hyp_state);
    trace("Unknown exception in Hyp\n");
    assert(hsr_ec_valid(hyp_state->hsr, HYP_EXCEPTION_UNKNOWN), "HSR EC mismatch");
    return HYP_ACTION_HALT;
}

HypExceptAction handle_prefetch_abort(HypExceptState *hyp_state) { 
    hyp_dump_unexpected_exception_state(hyp_state);
    trace("Prefetch abort in Hyp\n");
    assert(hsr_ec_valid(hyp_state->hsr, HYP_EXCEPTION_PREFETCH_ABORT), "HSR EC mismatch");
    return HYP_ACTION_HALT;
}

HypExceptAction handle_data_abort(HypExceptState *hyp_state) { 
    hyp_dump_unexpected_exception_state(hyp_state);
    trace("Data abort in Hyp\n");
    assert(hsr_ec_valid(hyp_state->hsr, HYP_EXCEPTION_DATA_ABORT), "HSR EC mismatch");
    return HYP_ACTION_HALT;
}

HypExceptAction handle_lower_sync(HypExceptState *hyp_state) { 
    assert(hsr_ec_valid(hyp_state->hsr, HYP_EXCEPTION_LOWER_SYNC), "HSR EC invalid");
    switch (HSR_EC(hyp_state->hsr)) { 
        case HSR_EC_HVC_A32: 
            return handle_hvc_from_lower(hyp_state);
        case HSR_EC_WFI_WFE:
            if ((HSR_ISS(hyp_state->hsr) & HSR_WFI_WFE_IS_WFE) != 0) {
                hyp_dump_unexpected_exception_state(hyp_state);
                trace("Unhandled WFE trap in Hyp\n");
                return HYP_ACTION_HALT;
            }
            return hv_scheduler_handle_wfi(&scheduler, hyp_state);
        case HSR_EC_PREFETCH_ABORT_LOWER:
        case HSR_EC_DATA_ABORT_LOWER:
            return handle_guest_abort(hyp_state);
        case HSR_EC_CP15_MCR_MRC:
            hyp_dump_unexpected_exception_state(hyp_state);
            trace("Unhandled guest CP15 trap: vcpu=%d iss=%p\n",
                  hv_scheduler_get_current(&scheduler) == 0 ? 0xffffffffu :
                  hv_scheduler_get_current(&scheduler)->id,
                  HSR_ISS(hyp_state->hsr));
            return HYP_ACTION_HALT;
        default:
            hyp_dump_unexpected_exception_state(hyp_state);
            trace("Unhandled lower-level trap in Hyp\n");
            return HYP_ACTION_HALT;
    }
}

HypExceptAction handle_irq(HypExceptState *hyp_state) { 
    // ARM generic timer path
    if (GEN_TIM_irq_pending()) {
        GEN_TIM_ack();
        counter++;

        HypExceptAction action = hv_scheduler_advance(&scheduler, hyp_state);
        GEN_TIM_rearm();
        return action;
    }
    // BCM timer path
    if (TIM_Check_IRQ()) {
        TIM_Clear_Pending();
        counter++;

        uint64_t now = TIM_SYS_Get_Ticks();
        // signal guest virtual timers
        bool delivered = deliver_expired_timers(now);
        hv_vtimer_rearm_physical(&scheduler);
        if (!delivered) {
            if (hv_scheduler_is_idle_vcpu(hv_scheduler_get_current(&scheduler))) {
                return hv_scheduler_advance(&scheduler, hyp_state);
            }
            return HYP_ACTION_RETURN;
        }

        hv_virq_sync(hv_scheduler_get_current(&scheduler));
        if (hv_scheduler_is_idle_vcpu(hv_scheduler_get_current(&scheduler))) {
            return hv_scheduler_advance(&scheduler, hyp_state);
        }
        return HYP_ACTION_RETURN;

    }
    hyp_dump_unexpected_exception_state(hyp_state);
    trace("Unexpected IRQ taken to Hyp\n");
    return HYP_ACTION_HALT;
}

// probably won't do fiqs for this project
HypExceptAction handle_fiq(HypExceptState *hyp_state) { 
    hyp_dump_unexpected_exception_state(hyp_state);
    trace("Unexpected FIQ taken to Hyp\n");
    return HYP_ACTION_HALT;
}
