#include "guest_hypercall.h"

void guest_main(void)
{
    if (guest_get_abi_version() != HYPERCALL_ABI_VERSION) {
        guest_exit(HYP_GUEST_EXIT_FAILURE);
    }

    if (guest_get_features() != HYP_FEATURE_BASE) {
        guest_exit(HYP_GUEST_EXIT_FAILURE);
    }

    guest_exit(HYP_GUEST_EXIT_SUCCESS);
}