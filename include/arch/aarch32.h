#pragma once

#define STACK_ADDR 0x8000000
#define IRQ_STACK_TOP 0x6000000
#define HYP_STACK_TOP 0x4000000

#define HCR_SWIO (1 << 1)

#define CPSR_PEMODE_MASK 0x1F

#define CPSR_PEMODE_USR 0x10
#define CPSR_PEMODE_FIQ 0x11
#define CPSR_PEMODE_IRQ 0x12
#define CPSR_PEMODE_SVC 0x13
#define CPSR_PEMODE_MON 0x16
#define CPSR_PEMODE_ABT 0x17
#define CPSR_PEMODE_HYP 0x1A
#define CPSR_PEMODE_UND 0x1B
#define CPSR_PEMODE_SYS 0x1F

#define CPSR_T (1u << 5) // Thumb mode
#define CPSR_F (1u << 6) // FIQ mask
#define CPSR_I (1u << 7) // IRQ mask
#define CPSR_A (1u << 8) // software error mask

#ifndef __ASSEMBLER__
#include <stdint.h>

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
