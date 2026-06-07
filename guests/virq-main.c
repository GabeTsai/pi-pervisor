#include <stdint.h>
#include "guest_hypercall.h"
#include "virq-abi.h"

volatile uint32_t guest_timer_ticks;

void guest_irq_handler(void) {
    int virq;

    while ((virq = guest_virq_claim()) >= 0) {
        if (virq == VIRQ_TIMER) {
            guest_timer_ticks++;
        }

        guest_virq_complete(virq);
    }
}

void guest_main(void) { 
    uint32_t features = guest_get_features();

    // make sure virtual IRQs are supported
    if ((features & HYP_FEATURE_VIRQ) == 0) {
        guest_exit(HYP_GUEST_EXIT_FAILURE);
    }

    // clear CSPR.I so virtual IRQs can be taken to guest
    clear_cpsr_bit(CPSR_I);

    guest_puts("guest_main start\n");
    while (guest_timer_ticks < 100) { }

    guest_exit(HYP_GUEST_EXIT_SUCCESS);
}