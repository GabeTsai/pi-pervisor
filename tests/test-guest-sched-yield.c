#include <stddef.h>
#include <stdint.h>
#include "aarch32.h"
#include "guest-image.h"
#include "hyp-enter-lower.h"
#include "hyp-regs.h"
#include "hv/scheduler.h"
#include "hv/vcpu.h"
#include "panic.h"
#include "uart.h"

extern uint8_t guest_sched0_image_start[];
extern uint8_t guest_sched0_image_end[];
extern uint8_t guest_sched1_image_start[];
extern uint8_t guest_sched1_image_end[];

static const GuestImage guest_images[] = {
    { 0, GUEST_SLOT_BASE(0), guest_sched0_image_start, guest_sched0_image_end },
    { 1, GUEST_SLOT_BASE(1), guest_sched1_image_start, guest_sched1_image_end },
};

static void load_guest_images(void) {
    for (uint32_t i = 0; i < sizeof(guest_images) / sizeof(guest_images[0]); i++) {
        const GuestImage *image = &guest_images[i];
        uint32_t image_size = (uint32_t)(image->image_end - image->image_start);
        uint8_t *dst = (uint8_t *)image->load_base;

        if (image->id >= HV_MAX_GUEST_VCPUS) {
            panic("guest id %d exceeds vCPU table", image->id);
        }

        if (image_size > GUEST_SLOT_SIZE) {
            panic("guest %d image too large: %d", image->id, image_size);
        }

        for (uint8_t *src = image->image_start; src < image->image_end; src++) {
            *dst++ = *src;
        }
    }

    DSB();
    ISB();
}

static void init_guest_vcpus(void) {
    for (uint32_t i = 0; i < sizeof(guest_images) / sizeof(guest_images[0]); i++) {
        const GuestImage *image = &guest_images[i];

        hv_vcpu_init(&scheduler.vcpus[image->id],
                     image->id,
                     image->load_base,
                     0,
                     HYP_LOWER_SVC_CPSR);
    }

    scheduler.cur_idx = 0;
    scheduler.cur_vcpu = &scheduler.vcpus[0];
    scheduler.vcpus[0].state = HV_VCPU_RUNNING;
    scheduler.vcpus[1].state = HV_VCPU_RUNNABLE;
}

void main(void) {
    UART_Init();

    load_guest_images();

    hv_scheduler_init(&scheduler);
    hv_scheduler_verbose = false;
    init_guest_vcpus();

    hv_vcpu_enter_initial(&scheduler.vcpus[0]);

    panic("should not get here");
}
