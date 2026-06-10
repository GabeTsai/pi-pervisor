#include <stdbool.h>
#include <stdint.h>

#include "check.h"
#include "hv/fault.h"
#include "hv/mmio.h"
#include "hv/vcpu.h"
#include "hv/vm.h"
#include "hyp-regs.h"
#include "panic.h"
#include "uart.h"

#define TEST_VALUE_VM0 0x12345678u
#define TEST_VALUE_VM1 0x87654321u

static HvVm vm0;
static HvVm vm1;
static HvVcpu vcpu0;
static HvVcpu vcpu1;

static void zero_hyp_state(HypExceptState *state) {
    uint32_t *word = (uint32_t *)state;

    for (uint32_t i = 0; i < sizeof(*state) / sizeof(uint32_t); i++) {
        word[i] = 0;
    }
}

static uint32_t abort_iss(uint32_t reg, uint32_t size, bool write, bool sign_extend) {
    uint32_t sas;

    switch (size) {
        case 1:
            sas = 0;
            break;
        case 2:
            sas = 1;
            break;
        case 4:
            sas = 2;
            break;
        default:
            sas = 3;
            break;
    }

    uint32_t iss = HSR_ISS_ABORT_ISV |
                   (sas << HSR_ISS_ABORT_SAS_SHIFT) |
                   (reg << HSR_ISS_ABORT_SRT_SHIFT);

    if (write) {
        iss |= HSR_ISS_ABORT_WNR;
    }

    if (sign_extend) {
        iss |= HSR_ISS_ABORT_SSE;
    }

    return iss;
}

static HvStage2FaultInfo fault_at(uint32_t offset, uint32_t iss, bool write) {
    HvStage2FaultInfo fault = {
        .vcpu_id = 0,
        .vm_id = 0,
        .ec = HSR_EC_DATA_ABORT_LOWER,
        .iss = iss,
        .status = 0,
        .level = 3,
        .ipa = HV_GUEST_UART_BASE + offset,
        .access = write ? HV_STAGE2_ACCESS_WRITE : HV_STAGE2_ACCESS_READ,
        .is_write = write,
        .is_instruction = false,
        .ipa_offset_valid = true,
    };

    return fault;
}

static void add_uart_region(HvVm *vm, uint32_t id) {
    int res = hv_vm_init(vm, id, id);
    assert(res == HV_VM_OK, "vm init failed");

    HvVmRegion uart = {
        .ipa_base = HV_GUEST_UART_BASE,
        .size = HV_GUEST_UART_SIZE,
        .pa_base = 0,
        .attrs = 0,
        .type = HV_VM_REGION_MMIO,
        .device = HV_VM_MMIO_DEVICE_VUART,
    };

    assert(hv_vm_add_region(vm, &uart) == HV_VM_OK, "add uart mmio failed");
    assert(hv_stage2_translate(vm, HV_GUEST_UART_BASE, &(HvPa){0}) == HV_STAGE2_ERR_UNMAPPED,
           "uart mmio translated unexpectedly");
}

static void expect_store(HvVcpu *vcpu,
                         uint32_t reg,
                         uint32_t offset,
                         uint32_t size,
                         uint32_t value) {
    HypExceptState state;
    HvStage2FaultInfo fault = fault_at(offset, abort_iss(reg, size, true, false), true);

    zero_hyp_state(&state);
    state.r[reg] = value;
    assert(hv_mmio_handle_fault(vcpu, &state, &fault) == HV_MMIO_OK,
           "mmio store failed");
}

static uint32_t expect_load(HvVcpu *vcpu,
                            uint32_t reg,
                            uint32_t offset,
                            uint32_t size,
                            bool sign_extend) {
    HypExceptState state;
    HvStage2FaultInfo fault = fault_at(offset, abort_iss(reg, size, false, sign_extend), false);

    zero_hyp_state(&state);
    assert(hv_mmio_handle_fault(vcpu, &state, &fault) == HV_MMIO_OK,
           "mmio load failed");
    return state.r[reg];
}

void main(void) {
    UART_Init();

    add_uart_region(&vm0, 0);
    add_uart_region(&vm1, 1);

    vcpu0.id = 0;
    vcpu0.vm = &vm0;
    vcpu1.id = 1;
    vcpu1.vm = &vm1;

    expect_store(&vcpu0, 3, HV_GUEST_AUX_MU_SCRATCH, 4, TEST_VALUE_VM0);
    assert(expect_load(&vcpu0, 4, HV_GUEST_AUX_MU_SCRATCH, 4, false) == TEST_VALUE_VM0,
           "vm0 scratch readback failed");

    expect_store(&vcpu1, 3, HV_GUEST_AUX_MU_SCRATCH, 4, TEST_VALUE_VM1);
    assert(expect_load(&vcpu1, 4, HV_GUEST_AUX_MU_SCRATCH, 4, false) == TEST_VALUE_VM1,
           "vm1 scratch readback failed");
    assert(expect_load(&vcpu0, 4, HV_GUEST_AUX_MU_SCRATCH, 4, false) == TEST_VALUE_VM0,
           "vm0 scratch leaked across VM");

    expect_store(&vcpu0, 5, HV_GUEST_AUX_MU_SCRATCH, 1, 0x80u);
    assert(expect_load(&vcpu0, 6, HV_GUEST_AUX_MU_SCRATCH, 1, true) == 0xffffff80u,
           "signed byte load did not extend");

    HypExceptState bad_state;
    HvStage2FaultInfo bad_fault = fault_at(HV_GUEST_AUX_MU_SCRATCH,
                                           abort_iss(13, 4, false, false),
                                           false);
    zero_hyp_state(&bad_state);
    assert(hv_mmio_handle_fault(&vcpu0, &bad_state, &bad_fault) == HV_MMIO_ERR_UNHANDLED,
           "sp target accepted");

    bad_fault = fault_at(0xff0u, abort_iss(1, 4, false, false), false);
    assert(hv_mmio_handle_fault(&vcpu0, &bad_state, &bad_fault) == HV_MMIO_ERR_UNHANDLED,
           "unknown uart offset accepted");

    bad_fault = fault_at(HV_GUEST_AUX_MU_SCRATCH, abort_iss(1, 4, false, false), false);
    bad_fault.iss &= ~HSR_ISS_ABORT_ISV;
    assert(hv_mmio_handle_fault(&vcpu0, &bad_state, &bad_fault) == HV_MMIO_ERR_UNHANDLED,
           "invalid syndrome accepted");

    trace("test-hv-mmio passed\n");
}
