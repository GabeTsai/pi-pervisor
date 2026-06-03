#include "aarch32.h"
#include "lower-hvc-test.h"

.section ".text"

.global lower_entry

lower_entry:
    mov sp, r0

    @ HVC to enter Hyp mode from SVC
    mov r0, #LOWER_HVC_TEST_PING
    mrs r1, cpsr @ CPSR, or PSTATE should be accessible from spsr_hyp when in hyp mode

    ldr r2, =0x12345678
    push {r2}
    mov r2, #0
    pop {r2}

    hvc #LOWER_HVC_TEST_PING_IMM

    @ Hyp handler returns known value
    ldr r1, =LOWER_HVC_TEST_RETURN_MAGIC
    cmp r0, r1
    bne failure

pass:
    mov r0, #LOWER_HVC_TEST_PASS
    hvc #0
    b pass 

failure:
    mov r0, #LOWER_HVC_TEST_FAIL
    hvc #0
    b failure

.section ".bss"

.balign 8
.global lower_test_stack
lower_test_stack:
    .space 4096

.global lower_stack_top
lower_stack_top:
