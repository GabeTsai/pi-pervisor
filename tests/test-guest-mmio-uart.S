.section ".rodata"

.balign 4
.global guest_mmio_uart0_image_start
guest_mmio_uart0_image_start:
    .incbin "../guests/guest-mmio-uart0.bin"

.balign 4
.global guest_mmio_uart0_image_end
guest_mmio_uart0_image_end:

.balign 4
.global guest_mmio_uart1_image_start
guest_mmio_uart1_image_start:
    .incbin "../guests/guest-mmio-uart1.bin"

.balign 4
.global guest_mmio_uart1_image_end
guest_mmio_uart1_image_end:
