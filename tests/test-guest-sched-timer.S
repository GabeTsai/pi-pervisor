.section ".rodata"

.balign 4
.global guest_timer0_image_start
guest_timer0_image_start:
    .incbin "../guests/guest-timer0.bin"

.balign 4
.global guest_timer0_image_end
guest_timer0_image_end:

.balign 4
.global guest_timer1_image_start
guest_timer1_image_start:
    .incbin "../guests/guest-timer1.bin"

.balign 4
.global guest_timer1_image_end
guest_timer1_image_end:
