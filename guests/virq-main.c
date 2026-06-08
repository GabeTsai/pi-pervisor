#include <stdint.h>
#include "guest_hypercall.h"
#include "printk.h"
#include "virq-abi.h"

// basic single-vCPU test for scheduling infrastructure and virtual timer interrupts
// should idle in dummy vCPU's wfi loop after code finishes
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

    printk("guest_main start\n");
    while (guest_timer_ticks < 300) { }

    printk("second round of waiting\n");
    while (guest_timer_ticks < 600) { }

    guest_exit(HYP_GUEST_EXIT_SUCCESS);
}
