#pragma once

#include <stdint.h>
#include "hv/hypercall.h"

// This header defines the hypercall API for the guest.

uint32_t guest_hypercall0(uint32_t call);
uint32_t guest_hypercall1(uint32_t call, uint32_t arg0);
uint32_t guest_hypercall2(uint32_t call, uint32_t arg0, uint32_t arg1);
uint32_t guest_hypercall3(uint32_t call, uint32_t arg0, uint32_t arg1, uint32_t arg2);

static inline uint32_t guest_get_features(void) {
    return guest_hypercall0(HYPERCALL_GET_FEATURES);
}

static inline uint32_t guest_get_abi_version(void) {
    return guest_hypercall0(HYPERCALL_GET_ABI_VERSION);
}

static inline void guest_exit(uint32_t status) {
    guest_hypercall1(HYPERCALL_EXIT, status);
    while (1) {}
}
