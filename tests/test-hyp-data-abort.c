#include <stdint.h>

#include "aarch32.h"
#include "check.h"
#include "printk.h"
#include "uart.h"

#define HSCTLR_A (1 << 1)

// triggers data abort by writing to an unaligned addres, 
// checks that data abort hyp exception handler is taken and handles correctly
void main(void)
{
    UART_Init();
    printk("test-hyp-data-abort start\n");
    printk("mode: %b\n", read_cpsr() & 0x1F);

    assert((read_cpsr() & 0x1F) == (PE_MODE_HYP | 0b10000), "bootloader was not in Hyp mode");
    assert((read_hcr() & 0x1F) == HCR_SWIO, "HCR was not initialized");

    printk("triggering data abort\n");
    volatile uint32_t *garbage = (volatile uint32_t *)0xDEADBEEF;
    *garbage = 0x676767;
}
