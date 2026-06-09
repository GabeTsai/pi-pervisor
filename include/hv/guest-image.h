#pragma once

#include <stdint.h>

// Guests are loaded into phys mem at this base address
#define GUEST_BASE 0x20000
#define GUEST_SLOT_SIZE 0x10000
#define GUEST_SLOT_BASE(slot) (GUEST_BASE + ((slot) * GUEST_SLOT_SIZE))

typedef struct {
    uint32_t id;
    uint32_t load_base;
    uint8_t *image_start;
    uint8_t *image_end;
} GuestImage;