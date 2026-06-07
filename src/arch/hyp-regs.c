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

uint32_t hsr_iss_imm16(uint32_t hsr) { 
    return HSR_ISS(hsr) & 0xffff;
}