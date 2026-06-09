#include <stdint.h>
#include "hyp-regs.h"

uint32_t read_hcr(void) { 
    uint32_t hcr;
    asm volatile ("mrc p15, 4, %0, c1, c1, 0" : "=r"(hcr));
    return hcr;
}

void write_hcr(uint32_t value) { 
    asm volatile ("mcr p15, 4, %0, c1, c1, 0" : : "r"(value));
}

void set_hcr_bit(uint32_t bit) { 
    uint32_t hcr = read_hcr();
    hcr |= bit;
    write_hcr(hcr);
}

void clear_hcr_bit(uint32_t bit) { 
    uint32_t hcr = read_hcr();
    hcr &= ~bit;
    write_hcr(hcr);
}

uint32_t read_vbar(void) {
    uint32_t vbar;
    asm volatile ("mrc p15, 0, %0, c12, c0, 0" : "=r"(vbar));
    return vbar;
}

void write_vbar(uint32_t value) {
    asm volatile (
        "mcr p15, 0, %0, c12, c0, 0\n"
        "isb"
        :
        : "r"(value)
        : "memory");
}

uint32_t read_vtcr(void) {
    uint32_t vtcr;
    asm volatile ("mrc p15, 4, %0, c2, c1, 2" : "=r"(vtcr));
    return vtcr;
}

void write_vtcr(uint32_t value) {
    asm volatile ("mcr p15, 4, %0, c2, c1, 2" : : "r"(value) : "memory");
}

uint64_t read_vttbr(void) {
    uint32_t low;
    uint32_t high;
    asm volatile ("mrrc p15, 6, %0, %1, c2" : "=r"(low), "=r"(high));
    return ((uint64_t)high << 32) | low;
}

void write_vttbr(uint64_t value) {
    uint32_t low = (uint32_t)value;
    uint32_t high = (uint32_t)(value >> 32);
    asm volatile ("mcrr p15, 6, %0, %1, c2" : : "r"(low), "r"(high) : "memory");
}

void hyp_invalidate_stage2_tlb_all(void) {
    uint32_t zero = 0;
    asm volatile ("mcr p15, 4, %0, c8, c7, 4" : : "r"(zero) : "memory");
}

void hyp_invalidate_stage2_tlb_ipa(uint64_t ipa) {
    uint32_t ipa_page = (uint32_t)(ipa >> 12);
    asm volatile ("mcr p15, 4, %0, c8, c4, 1" : : "r"(ipa_page) : "memory");
}

uint32_t hsr_iss_imm16(uint32_t hsr) { 
    return HSR_ISS(hsr) & 0xffff;
}
