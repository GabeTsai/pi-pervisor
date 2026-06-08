.section ".text"

.global hv_idle_vcpu_entry
.type hv_idle_vcpu_entry, %function

hv_idle_vcpu_entry:
    wfi
    b hv_idle_vcpu_entry

.size hv_idle_vcpu_entry, . - hv_idle_vcpu_entry
