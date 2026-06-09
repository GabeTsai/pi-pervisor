#include <stdint.h>
#include "guest-image.h"
#include "guest_hypercall.h"
#include "printk.h"

#define GUEST_STAGE2_GUARD_PAGE (GUEST_BASE + GUEST_SLOT_SIZE - 0x1000u)

// simple single-guest test that touches page in stage-2 memory that is not mapped, 
// which should trigger data abort and switch back to idle vCPU
void guest_main(void)
{
    volatile uint32_t *guard = (volatile uint32_t *)GUEST_STAGE2_GUARD_PAGE;

    printk("guest stage2 fault: hello from mapped RAM\n");

    if (guest_get_abi_version() != HYPERCALL_ABI_VERSION) {
        guest_exit(HYP_GUEST_EXIT_FAILURE);
    }

    printk("guest stage2 fault: touching guard page\n");
    (void)*guard;

    printk("guest stage2 fault: unexpected guard access success\n");
    guest_exit(HYP_GUEST_EXIT_FAILURE);
}
