#include <stdint.h>
#include "aarch32.h"
#include "guest_hypercall.h"
#include "printk.h"
#include "virq-abi.h"

#ifndef GUEST_ID
#error "GUEST_ID must be defined"
#endif

#define GUEST_TIMER_TARGET_TICKS 100

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

void guest_main(void)
{
    uint32_t features = guest_get_features();

    if ((features & HYP_FEATURE_VIRQ) == 0 ||
        (features & HYP_FEATURE_TIMER) == 0) {
        guest_exit(HYP_GUEST_EXIT_FAILURE);
    }

    clear_cpsr_bit(CPSR_I);

    printk("guest %d: timer start\n", GUEST_ID);
    while (guest_timer_ticks < GUEST_TIMER_TARGET_TICKS) {
        asm volatile ("wfi" ::: "memory");
    }

    printk("guest %d: timer done %d\n", GUEST_ID, guest_timer_ticks);
    guest_exit(HYP_GUEST_EXIT_SUCCESS);
}
