#include "guest_hypercall.h"
#include "aarch32.h"

void guest_main(void)
{
    guest_puts("hello from guest\n");

    if (guest_get_abi_version() != HYPERCALL_ABI_VERSION) {
        guest_exit(HYP_GUEST_EXIT_FAILURE);
    }

    if ((guest_get_features() & HYP_FEATURE_BASE) == 0) {
        guest_exit(HYP_GUEST_EXIT_FAILURE);
    }

    guest_exit(HYP_GUEST_EXIT_SUCCESS);
}
