#include <stdint.h>

#include "guest_hypercall.h"
#include "hyp-enter-lower.h"
#include "hyp-regs.h"
#include "panic.h"
#include "printk.h"
#include "timer.h"
#include "uart.h"

// Tests that guest can access Hyp mode IRQs. 
// Sequence:
// Hyp configures timer interrupts, and sets HCR_IMO so IRQs are taken to Hyp
// instead of EL1. The guest code repeatedly checks IRQ count
// with a custom hypercall, and exits when the count reaches a certain value. 
// all IRQs are running on the Hyp mode timer!
#define EXPECTED_HYP_IRQS 100

extern void guest_lower_entry(void);
extern uint8_t guest_lower_stack_top[];

void guest_main(void) {
    uint32_t count = 0;

    printk("guest_main start\n");
    while (count < EXPECTED_HYP_IRQS) {
        count = guest_get_hyp_irq_count();
    }

    guest_exit(HYP_GUEST_EXIT_SUCCESS);
}

void main(void) {
    UART_Init();

    TIM_Clear_Pending();
    TIM_Enable();
    TIM_Set_Frequency(100);
    TIM_Clear_Pending();
    TIM_Enable_IRQ();

    // we need this or else IRQs are taken to EL1, and the counter is never incremented
    hyp_enable_irq_routing();

    printk("test-hyp-irq start\n");
    printk("hcr: %b\n", read_hcr());

    hyp_enter_lower_mode(
        guest_lower_entry,
        (uint32_t)guest_lower_stack_top,
        HYP_LOWER_SVC_CPSR
    );

    panic("should not get here");
}
