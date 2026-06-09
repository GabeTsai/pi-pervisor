#pragma once

// Guests are loaded into phys mem at this base address
#define GUEST_BASE 0x20000
#define GUEST_SLOT_SIZE 0x10000
#define GUEST_SLOT_BASE(slot) (GUEST_BASE + ((slot) * GUEST_SLOT_SIZE))
