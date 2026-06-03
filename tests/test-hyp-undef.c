#include "aarch32.h"
#include "hyp-regs.h"
#include "check.h"
#include "printk.h"
#include "uart.h"

void main(void)
{
    UART_Init();
    printk("test-hyp-undef start\n");
    printk("mode: %b\n", read_cpsr() & 0x1F);

    assert((read_cpsr() & CPSR_PEMODE_MASK) == CPSR_PEMODE_HYP, "bootloader was not in Hyp mode");
    assert((read_hcr() & 0x1F) == HCR_SWIO, "HCR was not initialized");

    printk("triggering undefined instruction\n");
    asm volatile ("udf #0");

    assert(false, "undefined instruction returned");
}
