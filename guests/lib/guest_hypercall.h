#pragma once

#include <stdint.h>
#include "hv/hypercall.h"

// This header defines the hypercall API for the guest.
// The guest calls these functions to interact with they hypervisor.

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

static inline uint32_t guest_get_hyp_irq_count(void) {
    return guest_hypercall0(HYPERCALL_GET_HYP_IRQ_COUNT);
}

static inline void guest_exit(uint32_t status) {
    guest_hypercall1(HYPERCALL_EXIT, status);
    while (1) {}
}

static inline void guest_putchar(uint8_t c) {
    guest_hypercall1(HYPERCALL_PUTCHAR, (uint32_t)c);
}

static inline void guest_puts(const char *s) { 
    while (*s) {
        guest_putchar(*s++);
    }
}

static inline int guest_virq_claim(void) {
    return (int32_t)guest_hypercall0(HYPERCALL_VIRQ_CLAIM);
}

static inline int guest_virq_complete(int virq) {
    return (int32_t)guest_hypercall1(HYPERCALL_VIRQ_COMPLETE, (uint32_t)virq);
}
