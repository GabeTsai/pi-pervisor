#pragma once

#include <stdint.h>

// This header file defines the long-descriptor format for stage 2 translation table
// and the stage 2 translation table pool  

typedef uint64_t HvStage2Desc;
typedef uint64_t HvIpa;
typedef uint64_t HvPa;

#define HV_STAGE2_PAGE_SIZE      0x1000u // 4KB page size
#define HV_STAGE2_DESCS_PER_TABLE  512u // number of long-descriptors in one page table
#define HV_STAGE2_MAX_TABLE_PAGES 8u

/*
 * V1 uses a 32-bit IPA range, but the public types are 64-bit so the
 * stage-2 API can grow to the architectural 40-bit IPA/PA format.
 * L1 index: IPA[31:30]
 * L2 index: IPA[29:21]
 * L3 index: IPA[20:12]
 * offset:   IPA[11:0]
 */
#define HV_STAGE2_L1_SHIFT       30u
#define HV_STAGE2_L2_SHIFT       21u
#define HV_STAGE2_L3_SHIFT       12u
// up to 512 64-bit long descriptors per table
#define HV_STAGE2_INDEX_MASK     0x1ffu 

#define HV_STAGE2_OK             0
#define HV_STAGE2_ERR_INVAL      -1
#define HV_STAGE2_ERR_RANGE      -2
#define HV_STAGE2_ERR_NO_TABLES  -3
#define HV_STAGE2_ERR_MAPPED     -4

// Lower two bits of the descriptor
#define HV_STAGE2_DESC_INVALID   0b00ull // invalid descriptor
#define HV_STAGE2_DESC_BLOCK     0b01ull // block descriptor
#define HV_STAGE2_DESC_TABLE     0b11ull // table descriptor
#define HV_STAGE2_DESC_PAGE      0b11ull // page descriptor

// bits 12 to 39 of a Level 1/2 Table long-descriptor hold the next-level table address
#define HV_STAGE2_DESC_ADDR_MASK        0x000000fffffff000ull

// Stage-2 block/page descriptor attribute fields.
#define HV_STAGE2_AF_CLEAR              (0ull << 10)
#define HV_STAGE2_AF_SET                (1ull << 10)

#define HV_STAGE2_SH_NON_SHAREABLE      (0ull << 8)
#define HV_STAGE2_SH_RESERVED           (1ull << 8)
#define HV_STAGE2_SH_OUTER_SHAREABLE    (2ull << 8)
#define HV_STAGE2_SH_INNER_SHAREABLE    (3ull << 8)

#define HV_STAGE2_S2AP_NONE             (0ull << 6)
#define HV_STAGE2_S2AP_RO               (1ull << 6)
#define HV_STAGE2_S2AP_WO               (2ull << 6)
#define HV_STAGE2_S2AP_RW               (3ull << 6)

#define HV_STAGE2_MEMATTR_DEVICE_NGNRNE              (0x0ull << 2)
#define HV_STAGE2_MEMATTR_DEVICE_NGNRE               (0x1ull << 2)
#define HV_STAGE2_MEMATTR_DEVICE_NGRE                (0x2ull << 2)
#define HV_STAGE2_MEMATTR_DEVICE_GRE                 (0x3ull << 2)
#define HV_STAGE2_MEMATTR_NORMAL_OUTER_NC_INNER_RSVD (0x4ull << 2)
#define HV_STAGE2_MEMATTR_NORMAL_OUTER_NC_INNER_NC   (0x5ull << 2)
#define HV_STAGE2_MEMATTR_NORMAL_OUTER_NC_INNER_WT   (0x6ull << 2)
#define HV_STAGE2_MEMATTR_NORMAL_OUTER_NC_INNER_WB   (0x7ull << 2)
#define HV_STAGE2_MEMATTR_NORMAL_OUTER_WT_INNER_RSVD (0x8ull << 2)
#define HV_STAGE2_MEMATTR_NORMAL_OUTER_WT_INNER_NC   (0x9ull << 2)
#define HV_STAGE2_MEMATTR_NORMAL_OUTER_WT_INNER_WT   (0xaull << 2)
#define HV_STAGE2_MEMATTR_NORMAL_OUTER_WT_INNER_WB   (0xbull << 2)
#define HV_STAGE2_MEMATTR_NORMAL_OUTER_WB_INNER_RSVD (0xcull << 2)
#define HV_STAGE2_MEMATTR_NORMAL_OUTER_WB_INNER_NC   (0xdull << 2)
#define HV_STAGE2_MEMATTR_NORMAL_OUTER_WB_INNER_WT   (0xeull << 2)
#define HV_STAGE2_MEMATTR_NORMAL_OUTER_WB_INNER_WB   (0xfull << 2)

#define HV_STAGE2_AF                    HV_STAGE2_AF_SET
#define HV_STAGE2_MEMATTR_NORMAL_NC     HV_STAGE2_MEMATTR_NORMAL_OUTER_NC_INNER_NC
#define HV_STAGE2_PAGE_NORMAL_NC_RW     (HV_STAGE2_DESC_PAGE | \
                                         HV_STAGE2_AF_SET | \
                                         HV_STAGE2_SH_NON_SHAREABLE | \
                                         HV_STAGE2_S2AP_RW | \
                                         HV_STAGE2_MEMATTR_NORMAL_OUTER_NC_INNER_NC)

// VTCR defaults for a 32-bit IPA range starting stage-2 lookup at level 1.
#define HV_STAGE2_VTCR_T0SZ_4GB              0u
#define HV_STAGE2_VTCR_SL0_LEVEL1            (1u << 6)
#define HV_STAGE2_VTCR_IRGN0_NON_CACHEABLE   (0u << 8)
#define HV_STAGE2_VTCR_ORGN0_NON_CACHEABLE   (0u << 10)
#define HV_STAGE2_VTCR_SH0_NON_SHAREABLE     (0u << 12)
#define HV_STAGE2_VTCR_DEFAULT               (HV_STAGE2_VTCR_T0SZ_4GB | \
                                              HV_STAGE2_VTCR_SL0_LEVEL1 | \
                                              HV_STAGE2_VTCR_IRGN0_NON_CACHEABLE | \
                                              HV_STAGE2_VTCR_ORGN0_NON_CACHEABLE | \
                                              HV_STAGE2_VTCR_SH0_NON_SHAREABLE)

// table pages come from a hypervisor-owned page-table pool
typedef struct {
    HvStage2Desc *root_va;
    HvPa root_pa;
    HvStage2Desc *pool_va;
    HvPa pool_pa;
    uint32_t pool_pages;
    uint32_t pool_used;
} HvStage2;

struct HvVm;

// Initialize stage 2 translation tables for given virtual memory space
int hv_stage2_init(struct HvVm *vm);

/*
 *Map region of memory into the virtual memory space
 * vm: virtual memory space for a vCPU
 * ipa: intermediate physical address of region to map
 * pa: physical address of region to map
 * size: size of region to map in bytes
 * attrs: attributes of region to map
 */ 
int hv_stage2_map_region(struct HvVm *vm, HvIpa ipa, HvPa pa, uint64_t size, uint64_t attrs);

// Unmap page from the virtual memory space
int hv_stage2_unmap_page(struct HvVm *vm, HvIpa ipa);
