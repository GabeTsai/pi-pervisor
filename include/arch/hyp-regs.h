#pragma once

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

// all the bits for the ISS field for data/prefetch abort
// ISS for data/prefetch abort contains fault status code in lower 6 bits 
#define HSR_ISS_FAULT_STATUS_MASK 0x3f 
#define HSR_ISS_ABORT_WNR         (1u << 6)
// Indicates whether stage 2 fault was on access made for stage 1 page table walk
#define HSR_ISS_ABORT_S1PTW       (1u << 7)
#define HSR_ISS_ABORT_FNV         (1u << 10)
// Syndrom Register Transfer field of ISS - which guest register was involved in trapped abort
#define HSR_ISS_ABORT_SRT_SHIFT   16u
#define HSR_ISS_ABORT_SRT_MASK    0x1fu
#define HSR_ISS_ABORT_SSE         (1u << 21)
#define HSR_ISS_ABORT_SAS_SHIFT   22u
#define HSR_ISS_ABORT_SAS_MASK    0x3u
#define HSR_ISS_ABORT_ISV         (1u << 24)

#define HYP_BANKED_SP_USR    0
#define HYP_BANKED_SP_SVC    4
#define HYP_BANKED_LR_SVC    8
#define HYP_BANKED_SPSR_SVC  12
#define HYP_BANKED_SP_IRQ    16
#define HYP_BANKED_LR_IRQ    20
#define HYP_BANKED_SPSR_IRQ  24
#define HYP_BANKED_SIZE      28

// HCR bitmasks
#define HCR_VM   (1 << 0)
#define HCR_SWIO (1 << 1)
#define HCR_PTW  (1 << 2)
#define HCR_FMO  (1 << 3)
#define HCR_IMO  (1 << 4)
#define HCR_VI   (1 << 7)
#define HCR_TWI  (1 << 13)

// bit 1 of HSR ISS encoding for WFI/WFE determines if WFI/WFE trapped
#define HSR_WFI_WFE_IS_WFE (1 << 0)

// lower/same referring to relative level of execution exception was taken from
#ifndef __ASSEMBLER__

#include <stdint.h>

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

typedef struct {
    uint32_t sp_usr;
    uint32_t sp_svc;
    uint32_t lr_svc;
    uint32_t spsr_svc;
    uint32_t sp_irq;
    uint32_t lr_irq;
    uint32_t spsr_irq;
} HypBankedRegs;

#define VTTBR_VMID_SHIFT 48u
#define VTTBR_VMID_MASK  0xffull
#define VTTBR_BADDR_MASK 0x000000fffffff000ull

uint32_t read_hcr(void);
void write_hcr(uint32_t value);
void set_hcr_bit(uint32_t bit);
void clear_hcr_bit(uint32_t bit);
uint32_t read_vbar(void);
void write_vbar(uint32_t value);
uint32_t read_vtcr(void);
void write_vtcr(uint32_t value);
uint64_t read_vttbr(void);
void write_vttbr(uint64_t value);

// G6.2.132 - TLB Invalidate All, Non-Secure Non-Hyp
// Invalidates all cached copies of translation table entries from TLBs for any level of the translation table walk 
// required for stage 1 or 2.
void hyp_invalidate_stage2_tlb_all(void);
// G6.2.136 - TLB Invalidate by Intermediate Physical Address, Stage 2
// Stricter TLB invalidation 
void hyp_invalidate_stage2_tlb_ipa(uint64_t ipa);

static inline void hyp_enable_irq_routing(void) {
    set_hcr_bit(HCR_IMO);
}

uint32_t hsr_iss_imm16(uint32_t hsr);

// save/load banked registers for context switching between vcpus
void hyp_save_banked_regs(HypBankedRegs *banked_regs);
void hyp_load_banked_regs(const HypBankedRegs *banked_regs);
#endif
