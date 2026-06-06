#include "hv/hypercall.h"

.section ".text"

.global guest_hypercall0
.global guest_hypercall1
.global guest_hypercall2
.global guest_hypercall3

guest_hypercall0:
guest_hypercall1:
guest_hypercall2:
guest_hypercall3:
    hvc #HYPERCALL_HVC_IMM
    bx lr
