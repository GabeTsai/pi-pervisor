#include "aarch32.h"
#include "hv/hypercall.h"
.section ".text"

.global lower_entry

lower_entry:
    mov sp, r0

    @ HVC to enter Hyp mode from SVC
    mov r0, #HYPERCALL_GET_ABI_VERSION
    hvc #HYPERCALL_HVC_IMM

    cmp r0, #HYPERCALL_ABI_VERSION
    bne failure

pass:
    mov r0, #HYPERCALL_EXIT
    mov r1, #HYP_GUEST_EXIT_SUCCESS
    hvc #HYPERCALL_HVC_IMM
    bl clean_reboot

failure:
    mov r0, #HYPERCALL_EXIT
    mov r1, #HYP_GUEST_EXIT_FAILURE
    hvc #HYPERCALL_HVC_IMM
    bl clean_reboot

.section ".bss"

.balign 8
.global lower_test_stack
lower_test_stack:
    .space 4096

.global lower_stack_top
lower_stack_top:
