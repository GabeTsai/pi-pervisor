#include "guest/hypercall.h"
#include "hyp-enter-lower.h"
#include "panic.h"
#include "uart.h"

extern void guest_lower_entry(void);
extern uint8_t guest_lower_stack_top[];

void guest_main(void) {
    if (guest_get_abi_version() != HYPERCALL_ABI_VERSION) {
        guest_exit(HYP_GUEST_EXIT_FAILURE);
    }
    if (guest_get_features() != HYP_FEATURE_BASE) {
        guest_exit(HYP_GUEST_EXIT_FAILURE);
    }
    guest_exit(HYP_GUEST_EXIT_SUCCESS);
}

void main(void) {
    UART_Init();

    hyp_enter_lower_mode(
        guest_lower_entry,
        (uint32_t)guest_lower_stack_top,
        HYP_LOWER_SVC_CPSR
    );

    panic("should not get here");
}
