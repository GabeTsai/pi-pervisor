#include "mmio.h"

void PUT32(uint32_t location, uint32_t value) {
    asm volatile (
        "str r1, [r0]\n\t"
        "bx lr\n\t"
    );
}

uint32_t GET32(uint32_t location) {
    asm volatile (
        "ldr r0, [r0]\n\t"
        "bx lr\n\t"
    );
}

void put32(volatile uint32_t *addr, uint32_t value) {
    PUT32((uint32_t) addr, value);
}

uint32_t get32(volatile uint32_t *addr) {
    return GET32((uint32_t) addr);
}

void RMW_OR(uint32_t reg, uint32_t mask) {
    PUT32(reg, GET32(reg) | mask);
}

void RMW_AND(uint32_t reg, uint32_t mask) {
    PUT32(reg, GET32(reg) & mask);
}


uint32_t OR32(uint32_t addr, uint32_t x) {
    uint32_t v = GET32(addr);
    v |= x;
    PUT32(addr,v);
    return v;
}

uint32_t or32(volatile void *addr, uint32_t x) {
    return OR32((uint32_t) addr, x);
}
