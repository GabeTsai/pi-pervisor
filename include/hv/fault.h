#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "hv/stage2.h"

typedef enum {
    HV_STAGE2_ACCESS_READ,
    HV_STAGE2_ACCESS_WRITE,
    HV_STAGE2_ACCESS_INSTRUCTION,
} HvStage2Access;

typedef struct {
    uint32_t vcpu_id;
    uint32_t vm_id;
    uint32_t ec;
    uint32_t iss;
    uint32_t status;
    uint32_t level;
    HvIpa ipa;
    HvStage2Access access;
    bool is_write;
    bool is_instruction;
    bool ipa_offset_valid;
} HvStage2FaultInfo;
