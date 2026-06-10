#include <stdint.h>

#include "guest_hypercall.h"
#include "hv/mmio-abi.h"

#ifndef GUEST_ID
#error "GUEST_ID must be defined"
#endif

// Example test of how a guest might use the virtual UART device driver
// to implement their own UART library. 

// guest MMIO stores/loads to the UART registers result in CPA raising a data abort to Hyp/EL2
// since stage-2 translation has no mapping for the corresponding IPA. 
// EL2 recognizes the IPA as a VM MMIO region (since it was initialized that way)
// EL2 emulates the register access by taking the byte the guest tried to write and calling UART_Put8
// UART_Put8 then writes the byte to the actual UART device. 
static void mmio_write32(uint32_t offset, uint32_t value) {
    volatile uint32_t *reg = (volatile uint32_t *)(HV_GUEST_UART_BASE + offset);
    *reg = value;
}

static uint32_t mmio_read32(uint32_t offset) {
    volatile uint32_t *reg = (volatile uint32_t *)(HV_GUEST_UART_BASE + offset);
    return *reg;
}

static void vuart_putc(char c) {
    while ((mmio_read32(HV_GUEST_AUX_MU_LSR_REG) & (1u << 5)) == 0) {
    }
    mmio_write32(HV_GUEST_AUX_MU_IO_REG, (uint32_t)c);
}

static void vuart_puts(const char *s) {
    while (*s != '\0') {
        if (*s == '\n') {
            vuart_putc('\r');
        }
        vuart_putc(*s++);
    }
}

static void complete_pending_virqs(void) {
    int virq;

    while ((virq = guest_virq_claim()) >= 0) {
        guest_virq_complete(virq);
    }
}

void guest_main(void) {
    uint32_t scratch = 0xa5000000u | GUEST_ID;

    mmio_write32(HV_GUEST_AUX_ENABLES, 1);
    mmio_write32(HV_GUEST_AUX_MU_CNTL_REG, 3);
    mmio_write32(HV_GUEST_AUX_MU_SCRATCH, scratch);

    if (mmio_read32(HV_GUEST_AUX_MU_SCRATCH) != scratch) {
        guest_exit(HYP_GUEST_EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < 10; i++) {
        vuart_puts("hello from guest ");
        vuart_putc('0' + GUEST_ID);
        vuart_puts("\n");

        if (guest_timer_delay_millis(2000) != HYPERCALL_SUCCESS) {
            guest_exit(HYP_GUEST_EXIT_FAILURE);
        }
        complete_pending_virqs();
    }

    guest_exit(HYP_GUEST_EXIT_SUCCESS);
}
