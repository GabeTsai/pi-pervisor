#include <stdint.h>

#include "generic_timer.h"
#include "guest_hypercall.h"
#include "hyp-enter-lower.h"
#include "hyp-regs.h"
#include "hv/scheduler.h"
#include "check.h"
#include "panic.h"
#include "printk.h"
#include "timer.h"
#include "uart.h"

#define EXPECTED_HYP_IRQS 100
#define HYP_LOWER_SVC_IRQ_CPSR (HYP_LOWER_SVC_CPSR & ~CPSR_I)
#define GEN_TIM_ISTATUS_TIMEOUT_US 20000

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
    hv_scheduler_init(&scheduler);

    TIM_Clear_Pending();
    GEN_TIM_init(100);
    GEN_TIM_enable();

    uint32_t start = TIM_SYS_Get_Ticks();
    while (!GEN_TIM_istatus() &&
           (TIM_SYS_Get_Ticks() - start) < GEN_TIM_ISTATUS_TIMEOUT_US) {
    }
    assert(GEN_TIM_istatus(), "generic timer ISTATUS did not become set");
    GEN_TIM_rearm();

    hyp_enable_irq_routing();

    printk("test-hyp-generic-timer start\n");
    printk("hcr: %b\n", read_hcr());

    hyp_enter_lower_mode(
        guest_lower_entry,
        (uint32_t)guest_lower_stack_top,
        HYP_LOWER_SVC_IRQ_CPSR
    );

    panic("should not get here");
}
