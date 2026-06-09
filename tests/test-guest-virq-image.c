#include <stdint.h>
#include "guest-image.h"
#include "hyp-enter-lower.h"
#include "hyp-regs.h"
#include "hv/scheduler.h"
#include "hv/vcpu.h"
#include "hv/virq.h"
#include "panic.h"
#include "timer.h"
#include "uart.h"

extern uint8_t guest_virq_image_start[];
extern uint8_t guest_virq_image_end[];

#define GUEST_TIMER_HZ 300
#define USEC_PER_SEC 1000000u

static void load_guest_virq_image(void) {
    uint8_t *dst = (uint8_t *)GUEST_BASE;

    for (uint8_t *src = guest_virq_image_start; src < guest_virq_image_end; src++) {
        *dst++ = *src;
    }

    DSB();
    ISB();
}

void main(void) {
    UART_Init();

    load_guest_virq_image();

    hv_scheduler_init(&scheduler);
    hv_virq_init(&virq_controller);

    TIM_Clear_Pending();

    hv_vcpu_timer_start_periodic(scheduler.cur_vcpu,
                                 TIM_SYS_Get_Ticks(),
                                 USEC_PER_SEC / GUEST_TIMER_HZ);
    hv_vtimer_rearm_physical(&scheduler);

    hyp_enable_irq_routing();

    hyp_enter_lower_mode(
        (HypLowerEntry)GUEST_BASE,
        0,
        HYP_LOWER_SVC_CPSR
    );

    panic("should not get here");
}
