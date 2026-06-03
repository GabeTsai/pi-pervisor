#pragma once

#include <stdint.h>

#define HSR_EC_SHIFT 26
#define HSR_EC_MASK  0x3f
#define HSR_EC(hsr)  (((hsr) >> HSR_EC_SHIFT) & HSR_EC_MASK)
#define EC_BIT(ec) (1ULL << (ec))

#define HSR_IL_SHIFT 25
#define HSR_IL_MASK  0x1
#define HSR_IL(hsr)  (((hsr) >> HSR_IL_SHIFT) & HSR_IL_MASK)

#define HSR_ISS_SHIFT 0
#define HSR_ISS_MASK  0x01ffffff
#define HSR_ISS(hsr)  (((hsr) >> HSR_ISS_SHIFT) & HSR_ISS_MASK)

// lower/same referring to relative level of execution exception was taken from
enum {
    HSR_EC_UNKNOWN                       = 0b000000,
    HSR_EC_WFI_WFE                       = 0b000001,
    HSR_EC_CP15_MCR_MRC                  = 0b000011,
    HSR_EC_CP15_MCRR_MRRC                = 0b000100,
    HSR_EC_CP14_MCR_MRC                  = 0b000101,
    HSR_EC_LDC_STC                       = 0b000110,
    HSR_EC_FP_ASIMD                      = 0b000111,
    HSR_EC_VMRS_ID_TRAP                  = 0b001000,
    HSR_EC_CP14_MRRC                     = 0b001100,
    HSR_EC_ILLEGAL_EXCEPTION_RETURN_A32  = 0b001110,
    HSR_EC_SVC_A32                       = 0b010001,
    HSR_EC_HVC_A32                       = 0b010010,
    HSR_EC_SMC_A32                       = 0b010011,
    HSR_EC_PREFETCH_ABORT_LOWER          = 0b100000,
    HSR_EC_PREFETCH_ABORT_SAME           = 0b100001,
    HSR_EC_PC_ALIGNMENT_FAULT            = 0b100010,
    HSR_EC_DATA_ABORT_LOWER              = 0b100100,
    HSR_EC_DATA_ABORT_SAME               = 0b100101,
};

static inline uint32_t read_hcr(void) { 
    uint32_t hcr;
    asm volatile ("mrc p15, 4, %0, c1, c1, 0" : "=r"(hcr));
    return hcr;
}

static inline uint32_t hsr_iss_imm16(uint32_t hsr) { 
    return HSR_ISS(hsr) & 0xffff;
}