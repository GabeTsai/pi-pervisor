#include "hyp-enter-lower.h"
#include "panic.h"
#include "uart.h"

extern void guest_lower_entry(void);
extern uint8_t guest_lower_stack_top[];

void main(void) {
    UART_Init();

    hyp_enter_lower_mode(
        guest_lower_entry,
        (uint32_t)guest_lower_stack_top,
        HYP_LOWER_SVC_CPSR
    );

    panic("should not get here");
}
