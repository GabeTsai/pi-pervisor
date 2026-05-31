#pragma once

#include <stdint.h>

void hyp_dump_exception_state(uint32_t hsr, uint32_t elr_hyp, uint32_t spsr_hyp);