#include <stdint.h>

#include "aarch32.h"
#include "hv/stage2.h"
#include "hv/vm.h"
#include "hyp-regs.h"

static HvStage2Desc stage2_table_pools[HV_MAX_VMS]
                                      [HV_STAGE2_MAX_TABLE_PAGES]
                                      [HV_STAGE2_DESCS_PER_TABLE]
    __attribute__((aligned(HV_STAGE2_PAGE_SIZE), section(".stage2_tables")));

// get index of long-descriptor in page table
static uint32_t stage2_page_index(HvIpa ipa, uint32_t shift) {
    return (ipa >> shift) & HV_STAGE2_INDEX_MASK;
}

// 4KB page size alignment
static int stage2_aligned(uint64_t value) {
    return (value & (HV_STAGE2_PAGE_SIZE - 1u)) == 0;
}

static int stage2_fits_v1_32bit(uint64_t value) {
    return value <= 0xffffffffull;
}

static int stage2_range_end(uint64_t base, uint64_t size, uint64_t *end) {
    if (size == 0 || base > UINT64_MAX - (size - 1u)) {
        return 0;
    }

    *end = base + size - 1u;
    return 1;
}

// Helper function to check if a particular mapping is contained within a region
static void stage2_zero_table(HvStage2Desc *table) {
    for (uint32_t i = 0; i < HV_STAGE2_DESCS_PER_TABLE; i++) {
        table[i] = HV_STAGE2_DESC_INVALID;
    }
}

// helper function that returns pointer to start of page table
static HvStage2Desc *stage2_table_at(HvStage2 *stage2, uint32_t page_table_idx) {
    return stage2->pool_va + (page_table_idx * HV_STAGE2_DESCS_PER_TABLE);
}

// returns physical address of start of page table
static HvPa stage2_table_pa(HvStage2 *stage2, uint32_t page_table_idx) {
    return stage2->pool_pa + (page_table_idx * HV_STAGE2_PAGE_SIZE);
}

static HvStage2Desc *stage2_alloc_table(HvStage2 *stage2) {
    if (stage2->pool_used >= stage2->pool_pages) {
        return 0;
    }

    HvStage2Desc *table = stage2_table_at(stage2, stage2->pool_used);
    stage2_zero_table(table);
    stage2->pool_used++;
    return table;
}

// helper function that takes table descriptor and returns pointer to start of next level table
static HvStage2Desc *stage2_desc_to_table(HvStage2 *stage2, HvStage2Desc desc) {
    if ((desc & HV_STAGE2_DESC_TABLE) != HV_STAGE2_DESC_TABLE) {
        return 0;
    }
    // get next-level table address from table descriptor
    HvPa pa = desc & HV_STAGE2_DESC_ADDR_MASK;
    if (pa < stage2->pool_pa || !stage2_aligned(pa - stage2->pool_pa)) {
        return 0;
    }

    uint32_t page_idx = (uint32_t)((pa - stage2->pool_pa) / HV_STAGE2_PAGE_SIZE);
    if (page_idx >= stage2->pool_pages) {
        return 0;
    }

    return stage2_table_at(stage2, page_idx);
}

// either allocate a new next-level table or return pointer to existing next-level table
static HvStage2Desc *stage2_next_table(HvStage2 *stage2, HvStage2Desc *entry) {
    if ((*entry & HV_STAGE2_DESC_TABLE) == HV_STAGE2_DESC_TABLE) {
        return stage2_desc_to_table(stage2, *entry);
    }

    if ((*entry & 1ull) != 0) {
        return 0;
    }

    uint32_t page_idx = stage2->pool_used;
    HvStage2Desc *next = stage2_alloc_table(stage2);
    if (next == 0) {
        return 0;
    }
    // creates new next-level table descriptor in entry pointing to the new next-level table above
    *entry = ((uint64_t)stage2_table_pa(stage2, page_idx) & HV_STAGE2_DESC_ADDR_MASK) |
             HV_STAGE2_DESC_TABLE;
    return next;
}

// alloc controls whether to create new next-level tables while doing the page table walk
static HvStage2Desc *stage2_walk_l3(HvStage2 *stage2, HvIpa ipa, int alloc) {
    HvStage2Desc *l1 = stage2->root_va;
    HvStage2Desc *l1e = &l1[stage2_page_index(ipa, HV_STAGE2_L1_SHIFT)];
    HvStage2Desc *l2 = alloc ? stage2_next_table(stage2, l1e) : stage2_desc_to_table(stage2, *l1e);
    if (l2 == 0) {
        return 0;
    }

    HvStage2Desc *l2e = &l2[stage2_page_index(ipa, HV_STAGE2_L2_SHIFT)];
    HvStage2Desc *l3 = alloc ? stage2_next_table(stage2, l2e) : stage2_desc_to_table(stage2, *l2e);
    if (l3 == 0) {
        return 0;
    }

    return &l3[stage2_page_index(ipa, HV_STAGE2_L3_SHIFT)];
}

static int stage2_region_contains_mapping(
    const HvVmRegion *region,
    HvIpa ipa,
    HvPa pa,
    uint64_t size) {

    if (region == 0 ||
        region->type != HV_VM_REGION_RAM ||
        size == 0 ||
        ipa < region->ipa_base ||
        size > region->size ||
        (ipa - region->ipa_base) > (region->size - size)) {
        return 0;
    }

    uint64_t offset = ipa - region->ipa_base;
    return pa == region->pa_base + offset;
}

int hv_stage2_init(HvVm *vm) {
    if (vm == 0 || vm->id >= HV_MAX_VMS) {
        return HV_STAGE2_ERR_INVAL;
    }

    HvStage2 *stage2 = &vm->stage2;
    stage2->pool_va = &stage2_table_pools[vm->id][0][0];
    stage2->pool_pa = (HvPa)(uintptr_t)stage2->pool_va;
    stage2->pool_pages = HV_STAGE2_MAX_TABLE_PAGES;
    stage2->pool_used = 1;
    stage2->root_va = stage2_table_at(stage2, 0);
    stage2->root_pa = stage2_table_pa(stage2, 0);

    for (uint32_t i = 0; i < stage2->pool_pages; i++) {
        stage2_zero_table(stage2_table_at(stage2, i));
    }

    DSB();
    return HV_STAGE2_OK;
}

static int stage2_map_page(HvVm *vm, HvIpa ipa, HvPa pa, uint64_t attrs) {
    HvStage2Desc *entry = stage2_walk_l3(&vm->stage2, ipa, 1);
    if (entry == 0) {
        return HV_STAGE2_ERR_NO_TABLES;
    }
    // if bit 0 is set, the page is already mapped - G4-4588
    if ((*entry & 1ull) != 0) {
        return HV_STAGE2_ERR_MAPPED;
    }

    *entry = ((uint64_t)pa & HV_STAGE2_DESC_ADDR_MASK) |
             (attrs & ~HV_STAGE2_DESC_ADDR_MASK) |
             HV_STAGE2_DESC_PAGE;
    return HV_STAGE2_OK;
}

int hv_stage2_map_region(HvVm *vm, HvIpa ipa, HvPa pa, uint64_t size, uint64_t attrs) {
    uint64_t ipa_end;
    uint64_t pa_end;

    if (vm == 0 || 
        !stage2_aligned(ipa) || 
        !stage2_aligned(pa) || 
        !stage2_aligned(size) || size == 0) {
        return HV_STAGE2_ERR_INVAL;
    }

    if (!stage2_range_end(ipa, size, &ipa_end) ||
        !stage2_range_end(pa, size, &pa_end)) {
        return HV_STAGE2_ERR_INVAL;
    }

    if (!stage2_fits_v1_32bit(ipa_end) ||
        !stage2_fits_v1_32bit(pa_end)) {
        return HV_STAGE2_ERR_RANGE;
    }

    const HvVmRegion *region = hv_vm_find_region(vm, ipa);
    if (!stage2_region_contains_mapping(region, ipa, pa, size)) {
        return HV_STAGE2_ERR_RANGE;
    }

    // map an equivalent number of 4KB pages to accomodate size bytes
    for (uint64_t offset = 0; offset < size; offset += HV_STAGE2_PAGE_SIZE) {
        int res = stage2_map_page(vm, ipa + offset, pa + offset, attrs);
        if (res != HV_STAGE2_OK) {
            hyp_invalidate_stage2_tlb_all();
            DSB();
            ISB();
            return res;
        }
    }

    hyp_invalidate_stage2_tlb_all();
    DSB();
    ISB();
    return HV_STAGE2_OK;
}

int hv_stage2_unmap_page(HvVm *vm, HvIpa ipa) {
    if (vm == 0 || !stage2_aligned(ipa)) {
        return HV_STAGE2_ERR_INVAL;
    }

    const HvVmRegion *region = hv_vm_find_region(vm, ipa);
    if (region == 0 ||
        ipa < region->ipa_base ||
        (ipa - region->ipa_base) > (region->size - HV_STAGE2_PAGE_SIZE)) {
        return HV_STAGE2_ERR_RANGE;
    }
    // find entry in the page table that corresponds to the given IPA, invalidate it
    HvStage2Desc *entry = stage2_walk_l3(&vm->stage2, ipa, 0);
    if (entry != 0) {
        *entry = HV_STAGE2_DESC_INVALID;
    }

    hyp_invalidate_stage2_tlb_ipa(ipa);
    DSB();
    ISB();
    return HV_STAGE2_OK;
}

// utility function to convert IPA to PA. 
// does full stage-2 translation walk to find PA and validates it.
int hv_stage2_translate(HvVm *vm, HvIpa ipa, HvPa *pa_out) {
    if (vm == 0 || pa_out == 0 || !stage2_fits_v1_32bit(ipa)) {
        return HV_STAGE2_ERR_INVAL;
    }

    HvStage2Desc *entry = stage2_walk_l3(&vm->stage2, ipa, 0);
    if (entry == 0 || (*entry & HV_STAGE2_DESC_PAGE) != HV_STAGE2_DESC_PAGE) {
        return HV_STAGE2_ERR_UNMAPPED;
    }

    *pa_out = (*entry & HV_STAGE2_DESC_ADDR_MASK) |
              (ipa & (HV_STAGE2_PAGE_SIZE - 1u));
    return HV_STAGE2_OK;
}
