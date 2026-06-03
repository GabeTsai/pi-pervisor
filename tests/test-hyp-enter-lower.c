#include "hyp-enter-lower.h"
#include "hyp-regs.h"
#include "uart.h"
#include "panic.h"

extern void lower_entry(void);
extern uint8_t lower_stack_top[];
// This test enters SVC from Hyp mode, installs appropriate state, then executes HVC
// back to Hyp mode. The hvc handler modifies the saved r0, and ERET 
// returns to the instruction after HVC. Checks if r0 was modified.
void main(void) { 
    UART_Init();

    // enter SVC from Hyp mode
    hyp_enter_lower_mode(
        lower_entry, 
        (uint32_t)lower_stack_top,
        HYP_LOWER_SVC_CPSR
    );

    panic("should not get here");
}