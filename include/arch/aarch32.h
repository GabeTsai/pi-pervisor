#pragma once

#define STACK_ADDR 0x8000000
#define IRQ_STACK_TOP 0x6000000
#define HYP_STACK_TOP 0x4000000

#ifndef __ASSEMBLER__
#include <stdint.h>

typedef enum { 
    PEMODE_USER = 0b0000, 
    PEMODE_FIQ = 0b0001, 
    PEMODE_IRQ = 0b0010, 
    PEMODE_SUPERVISOR = 0b0011, 
    PEMODE_MONITOR = 0b0110, 
    PEMODE_ABORT = 0b0111, 
    PE_MODE_HYP = 0b1010, 
    PE_MODE_UNDEFINED = 0b1011,
    PE_MODE_SYSTEM = 0b1111,
} PEModeT;

static inline void ISB(void) {
    asm volatile ("isb sy" ::: "memory");
}

static inline void DSB(void) {
    asm volatile ("dsb sy" ::: "memory");
}

static inline void DMB(void) {
    asm volatile ("dmb sy" ::: "memory");
}
static inline uint32_t read_cpsr(void) { 
    uint32_t cpsr;
    asm volatile ("mrs %0, cpsr" : "=r"(cpsr));
    return cpsr;
}
#endif
