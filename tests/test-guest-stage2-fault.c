#include <stdint.h>
#include "aarch32.h"
#include "guest-image.h"
#include "hyp-enter-lower.h"
#include "hyp-regs.h"
#include "hv/scheduler.h"
#include "hv/vcpu.h"
#include "panic.h"
#include "uart.h"

extern uint8_t guest_stage2_fault_image_start[];
extern uint8_t guest_stage2_fault_image_end[];

static void load_guest_image(void) {
    uint32_t image_size = (uint32_t)(guest_stage2_fault_image_end - guest_stage2_fault_image_start);
    uint8_t *dst = (uint8_t *)GUEST_BASE;

    if (image_size > GUEST_SLOT_SIZE) {
        panic("guest stage2 fault image too large: %d", image_size);
    }

    for (uint8_t *src = guest_stage2_fault_image_start; src < guest_stage2_fault_image_end; src++) {
        *dst++ = *src;
    }

    DSB();
    ISB();
}

void main(void) {
    UART_Init();

    load_guest_image();

    hv_scheduler_init(&scheduler);
    hv_scheduler_verbose = true;

    HvVcpu *boot_vcpu = &scheduler.vcpus[0];
    hv_vcpu_init(boot_vcpu, 0, GUEST_BASE, 0, HYP_LOWER_SVC_CPSR);
    scheduler.cur_idx = 0;
    scheduler.cur_vcpu = boot_vcpu;
    boot_vcpu->state = HV_VCPU_RUNNING;

    hv_vcpu_enter_initial(boot_vcpu);

    panic("should not get here");
}
