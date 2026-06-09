.section ".rodata"

.balign 4
.global guest_sched0_image_start
guest_sched0_image_start:
    .incbin "../guests/guest-sched0.bin"

.balign 4
.global guest_sched0_image_end
guest_sched0_image_end:

.balign 4
.global guest_sched1_image_start
guest_sched1_image_start:
    .incbin "../guests/guest-sched1.bin"

.balign 4
.global guest_sched1_image_end
guest_sched1_image_end:
