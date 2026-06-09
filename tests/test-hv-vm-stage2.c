#include <stdint.h>

#include "check.h"
#include "hv/stage2.h"
#include "hv/vm.h"
#include "panic.h"
#include "uart.h"

// rans a gamut of tests for various aspects of VM and stage-2 translation table functionality
// including:
// - proper VM init
// - VM regions being allowed to overlap
// - Unmapped VM regions found/not found as expected
// - Mapped VM regions translated as expected
// - Unmapped VM regions not able to be translated

#define TEST_IPA_BASE 0x400000u
#define TEST_PA_BASE  0x800000u
#define TEST_RAM_SIZE (3u * HV_STAGE2_PAGE_SIZE)
#define TEST_GUARD_IPA (TEST_IPA_BASE + TEST_RAM_SIZE)
#define TEST_MMIO_IPA 0x3f200000u

static HvVm vm;

static void expect_region(const HvVmRegion *region, HvVmRegionType type) {
    assert(region != 0, "expected VM region");
    assert(region->type == type, "unexpected VM region type");
}

void main(void) {
    UART_Init();

    int res = hv_vm_init(&vm, 0, 7);
    assert(res == HV_VM_OK, "hv_vm_init failed");
    assert(vm.id == 0, "VM slot id changed");
    assert(vm.vmid == 7, "VMID changed");

    HvVmRegion ram = {
        .ipa_base = TEST_IPA_BASE,
        .size = TEST_RAM_SIZE,
        .pa_base = TEST_PA_BASE,
        .attrs = HV_STAGE2_PAGE_NORMAL_NC_RW,
        .type = HV_VM_REGION_RAM,
    };
    assert(hv_vm_add_region(&vm, &ram) == HV_VM_OK, "add RAM failed");

    HvVmRegion overlap = {
        .ipa_base = TEST_IPA_BASE + HV_STAGE2_PAGE_SIZE,
        .size = HV_STAGE2_PAGE_SIZE,
        .pa_base = TEST_PA_BASE + HV_STAGE2_PAGE_SIZE,
        .attrs = HV_STAGE2_PAGE_NORMAL_NC_RW,
        .type = HV_VM_REGION_RAM,
    };
    assert(hv_vm_add_region(&vm, &overlap) == HV_VM_ERR_OVERLAP,
           "overlap accepted");

    HvVmRegion guard = {
        .ipa_base = TEST_GUARD_IPA,
        .size = HV_STAGE2_PAGE_SIZE,
        .pa_base = 0,
        .attrs = 0,
        .type = HV_VM_REGION_RAM_GUARD,
    };
    assert(hv_vm_add_region(&vm, &guard) == HV_VM_OK, "add guard failed");

    HvVmRegion mmio = {
        .ipa_base = TEST_MMIO_IPA,
        .size = HV_STAGE2_PAGE_SIZE,
        .pa_base = 0,
        .attrs = 0,
        .type = HV_VM_REGION_MMIO,
    };
    assert(hv_vm_add_region(&vm, &mmio) == HV_VM_OK, "add MMIO failed");
    // should be able to find mapped regions, but not unmapped ones
    expect_region(hv_vm_find_region(&vm, TEST_IPA_BASE), HV_VM_REGION_RAM);
    expect_region(hv_vm_find_region(&vm, TEST_GUARD_IPA), HV_VM_REGION_RAM_GUARD);
    expect_region(hv_vm_find_region(&vm, TEST_MMIO_IPA), HV_VM_REGION_MMIO);
    assert(hv_vm_find_region(&vm, TEST_MMIO_IPA - HV_STAGE2_PAGE_SIZE) == 0,
           "unexpected region found");

    assert(hv_vm_build_stage2(&vm) == HV_VM_OK, "build stage2 failed");

    HvPa pa;
    assert(hv_stage2_translate(&vm, TEST_IPA_BASE, &pa) == HV_STAGE2_OK,
           "RAM base did not translate");
    assert(pa == TEST_PA_BASE, "RAM base PA mismatch");

    assert(hv_stage2_translate(&vm, TEST_IPA_BASE + 0x234u, &pa) == HV_STAGE2_OK,
           "RAM offset did not translate");
    assert(pa == TEST_PA_BASE + 0x234u, "RAM offset PA mismatch");

    assert(hv_stage2_translate(&vm, TEST_GUARD_IPA, &pa) == HV_STAGE2_ERR_UNMAPPED,
           "guard translated unexpectedly");
    assert(hv_stage2_translate(&vm, TEST_MMIO_IPA, &pa) == HV_STAGE2_ERR_UNMAPPED,
           "MMIO translated unexpectedly");
    assert(hv_stage2_map_region(&vm,
                                TEST_GUARD_IPA,
                                TEST_PA_BASE,
                                HV_STAGE2_PAGE_SIZE,
                                HV_STAGE2_PAGE_NORMAL_NC_RW) == HV_STAGE2_ERR_RANGE,
           "guard mapping accepted");

    trace("test-hv-vm-stage2 passed\n");
}
