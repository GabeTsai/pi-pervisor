#include "hv/vuart.h"

#include "hv/mmio-abi.h"
#include "hv/mmio.h"
#include "hv/vm.h"
#include "uart.h"

#define VUART_LSR_RX_READY       (1u << 0)
#define VUART_LSR_TX_READY       (1u << 5)
#define VUART_LSR_TX_EMPTY       (1u << 6)
#define VUART_STAT_TX_CAN_ACCEPT (1u << 1)

typedef struct {
    uint32_t aux_enables;
    uint32_t ier;
    uint32_t iir;
    uint32_t lcr;
    uint32_t mcr;
    uint32_t msr;
    uint32_t scratch;
    uint32_t cntl;
    uint32_t stat;
    uint32_t baud;
} HvVirtualUart;

static HvVirtualUart vuarts[HV_MAX_VMS];

static int vuart_valid_access(uint32_t vm_id, uint32_t size) {
    return vm_id < HV_MAX_VMS && (size == 1 || size == 2 || size == 4);
}

int hv_vuart_write(uint32_t vm_id, uint32_t offset, uint32_t value, uint32_t size) {
    if (!vuart_valid_access(vm_id, size)) {
        return HV_MMIO_ERR_UNHANDLED;
    }

    HvVirtualUart *uart = &vuarts[vm_id];

    switch (offset) {
        case HV_GUEST_AUX_ENABLES:
            uart->aux_enables = value & 1u;
            return HV_MMIO_OK;
        case HV_GUEST_AUX_MU_IO_REG:
            UART_Put8((uint8_t)value);
            return HV_MMIO_OK;
        case HV_GUEST_AUX_MU_IER_REG:
            uart->ier = value;
            return HV_MMIO_OK;
        case HV_GUEST_AUX_MU_IIR_REG:
            uart->iir = value;
            return HV_MMIO_OK;
        case HV_GUEST_AUX_MU_LCR_REG:
            uart->lcr = value;
            return HV_MMIO_OK;
        case HV_GUEST_AUX_MU_MCR_REG:
            uart->mcr = value;
            return HV_MMIO_OK;
        case HV_GUEST_AUX_MU_SCRATCH:
            uart->scratch = value;
            return HV_MMIO_OK;
        case HV_GUEST_AUX_MU_CNTL_REG:
            uart->cntl = value;
            return HV_MMIO_OK;
        case HV_GUEST_AUX_MU_STAT_REG:
            uart->stat = value;
            return HV_MMIO_OK;
        case HV_GUEST_AUX_MU_BAUD_REG:
            uart->baud = value;
            return HV_MMIO_OK;
        default:
            return HV_MMIO_ERR_UNHANDLED;
    }
}

int hv_vuart_read(uint32_t vm_id, uint32_t offset, uint32_t size, uint32_t *value) {
    if (!vuart_valid_access(vm_id, size) || value == 0) {
        return HV_MMIO_ERR_UNHANDLED;
    }

    HvVirtualUart *uart = &vuarts[vm_id];

    switch (offset) {
        case HV_GUEST_AUX_ENABLES:
            *value = uart->aux_enables;
            return HV_MMIO_OK;
        case HV_GUEST_AUX_MU_IO_REG: {
            int c = UART_Get8_Async();
            *value = c < 0 ? 0 : (uint32_t)c;
            return HV_MMIO_OK;
        }
        case HV_GUEST_AUX_MU_IER_REG:
            *value = uart->ier;
            return HV_MMIO_OK;
        case HV_GUEST_AUX_MU_IIR_REG:
            *value = uart->iir;
            return HV_MMIO_OK;
        case HV_GUEST_AUX_MU_LCR_REG:
            *value = uart->lcr;
            return HV_MMIO_OK;
        case HV_GUEST_AUX_MU_MCR_REG:
            *value = uart->mcr;
            return HV_MMIO_OK;
        case HV_GUEST_AUX_MU_LSR_REG:
            *value = VUART_LSR_TX_READY | VUART_LSR_TX_EMPTY;
            if (UART_Has_Data()) {
                *value |= VUART_LSR_RX_READY;
            }
            return HV_MMIO_OK;
        case HV_GUEST_AUX_MU_MSR_REG:
            *value = uart->msr;
            return HV_MMIO_OK;
        case HV_GUEST_AUX_MU_SCRATCH:
            *value = uart->scratch;
            return HV_MMIO_OK;
        case HV_GUEST_AUX_MU_CNTL_REG:
            *value = uart->cntl;
            return HV_MMIO_OK;
        case HV_GUEST_AUX_MU_STAT_REG:
            *value = uart->stat | VUART_STAT_TX_CAN_ACCEPT;
            return HV_MMIO_OK;
        case HV_GUEST_AUX_MU_BAUD_REG:
            *value = uart->baud;
            return HV_MMIO_OK;
        default:
            return HV_MMIO_ERR_UNHANDLED;
    }
}
