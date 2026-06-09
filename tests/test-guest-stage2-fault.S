.section ".rodata"

.balign 4
.global guest_stage2_fault_image_start
guest_stage2_fault_image_start:
    .incbin "../guests/guest-stage2-fault.bin"

.balign 4
.global guest_stage2_fault_image_end
guest_stage2_fault_image_end:
