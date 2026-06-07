.section ".rodata"

.balign 4
.global guest_virq_image_start
guest_virq_image_start:
    .incbin "../guests/guest-virq.bin"

.balign 4
.global guest_virq_image_end
guest_virq_image_end:
