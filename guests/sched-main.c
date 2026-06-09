#include <stdint.h>
#include "guest_hypercall.h"
#include "printk.h"

// Makefile-determined guest ID
#ifndef GUEST_ID
#error "GUEST_ID must be defined"
#endif

void guest_main(void)
{
    for (uint32_t i = 0; i < 10; i++) {
        printk("guest %d: hello\n", GUEST_ID);
        guest_yield();
    }

    guest_exit(HYP_GUEST_EXIT_SUCCESS);
}
