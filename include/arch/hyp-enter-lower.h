#pragma once

#include "aarch32.h"

#include <stdint.h>

// IRQ, FIQ masked
#define HYP_LOWER_SVC_CPSR (CPSR_PEMODE_SVC | CPSR_A | CPSR_I | CPSR_F)
#define HYP_LOWER_SYS_CPSR (CPSR_PEMODE_SYS | CPSR_A | CPSR_I | CPSR_F)

typedef void (*HypLowerEntry)(void);

__attribute__((noreturn))
void hyp_enter_lower_mode(HypLowerEntry entry, uint32_t stack_top, uint32_t cpsr);
