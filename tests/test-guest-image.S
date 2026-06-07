.section ".rodata"

.balign 4
.global guest_image_start
guest_image_start:
    .incbin "../guests/guest.bin"

.balign 4
.global guest_image_end
guest_image_end:
