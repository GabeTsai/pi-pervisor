.section ".rodata"

.balign 4
.global guest0_stage2_fault_image_start
guest0_stage2_fault_image_start:
    .incbin "../guests/guest-stage2-fault.bin"

.balign 4
.global guest0_stage2_fault_image_end
guest0_stage2_fault_image_end:

.balign 4
.global guest1_stage2_fault_image_start
guest1_stage2_fault_image_start:
    .incbin "../guests/guest-stage2-fault.bin"

.balign 4
.global guest1_stage2_fault_image_end
guest1_stage2_fault_image_end:
