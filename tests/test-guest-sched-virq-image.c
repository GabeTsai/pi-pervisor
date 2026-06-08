#include <stdint.h>
#include "generic_timer.h"
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
    hv_scheduler_verbose = true;

    TIM_Clear_Pending();
    TIM_Enable();
    TIM_Set_Frequency(300);
    TIM_Clear_Pending();
    TIM_Enable_IRQ();

    GEN_TIM_init(100);
    GEN_TIM_enable();

    hyp_enable_irq_routing();

    HvVcpu *boot_vcpu = &scheduler.vcpus[0];
    hv_vcpu_init(boot_vcpu, 0, GUEST_BASE, 0, HYP_LOWER_SVC_CPSR);

    scheduler.cur_idx = 0;
    scheduler.cur_vcpu = boot_vcpu;
    boot_vcpu->state = HV_VCPU_RUNNING;

    hv_vcpu_enter_initial(boot_vcpu);

    panic("should not get here");
}
