#pragma once

#include "aarch32.h"

// unfortunately have to use a bunch of defines rather than enums so 
// the code is assembly-friendly
#define HYPERCALL_ABI_VERSION 1
#define HYPERCALL_HVC_IMM 0 // we use zero, per ARM SMCCC convention

// hypercall return values from EL2 back to guest
#define HYPERCALL_SUCCESS 0
#define HYPERCALL_ERR_UNKNOWN -1
#define HYPERCALL_ERR_INVALID -2
#define HYPERCALL_ERR_NO_PENDING -3

// bit masks that indicate all supported hypervisor features
#define HYP_FEATURE_BASE  (1 << 0)
#define HYP_FEATURE_VIRQ  (1 << 1)
#define HYP_FEATURE_TIMER (1 << 2)
#define HYP_FEATURE_UART  (1 << 3)

// hypercall numbers implemented by the hypervisor
#define HYPERCALL_GET_ABI_VERSION   0
#define HYPERCALL_GET_FEATURES      1
#define HYPERCALL_EXIT              2
#define HYPERCALL_YIELD             3
#define HYPERCALL_ADVANCE           4
#define HYPERCALL_PUTCHAR           5
#define HYPERCALL_GET_HYP_IRQ_COUNT 6
#define HYPERCALL_VIRQ_CLAIM        7
#define HYPERCALL_VIRQ_COMPLETE     8
#define HYPERCALL_TIMER_GET_TICKS   9
#define HYPERCALL_TIMER_DELAY_TICKS 10
#define HYPERCALL_TIMER_GET_FREQ    11

// guest exit statuses passed from guest to hypervisor
#define HYP_GUEST_EXIT_SUCCESS 0
#define HYP_GUEST_EXIT_FAILURE 1

#ifndef __ASSEMBLER__
#include "hyp-exceptions.h"

HypExceptAction hyp_handle_guest_hypercall(HypExceptState *hyp_state);

#endif
