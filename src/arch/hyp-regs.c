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

uint32_t hsr_iss_imm16(uint32_t hsr) { 
    return HSR_ISS(hsr) & 0xffff;
}
