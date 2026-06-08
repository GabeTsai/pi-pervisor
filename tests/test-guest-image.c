#include <stdint.h>
#include "aarch32.h"
#include "guest-image.h"
#include "hyp-enter-lower.h"
#include "hyp-regs.h"
#include "hv/scheduler.h"
#include "hv/vcpu.h"
#include "panic.h"
#include "uart.h"

// This test takes guest code (`guest.bin`) and sticks it into the test image as raw bytes.
// Then, manually copies those bytes into the physical address where guest linked. 
// Does the same typical EL2 to EL1 communication pathway: 

// Start in Hyp mode, enter EL1 guest code at GUEST_BASE, 
// (we do not set the stack since the linker script already does that for us)

// The vCPU scheduler should select the boot vCPU and run its code in guest_main until 
// it exits, where it should then eret into hv_idle_vcpu_entry and wait for next interrupt
extern uint8_t guest_image_start[];
extern uint8_t guest_image_end[];

// physically copy all bytes of guest image binary into memory at GUEST_BASE
static void load_guest_image(void) {
    uint8_t *dst = (uint8_t *)GUEST_BASE;

    for (uint8_t *src = guest_image_start; src < guest_image_end; src++) {
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
