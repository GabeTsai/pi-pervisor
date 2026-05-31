#include "uart.h"
#include "printk.h"
#include "aarch32.h"
#include "gpio.h"
#include "check.h"

// this test runs a dummy HVC/hypervisor call 
// check that HVC instruction works and uses HVBAR to jump to hyp vector table, runs hvc handler
// and returns back from handler
void main(void) {
    UART_Init();
    printk("mode: %b\n", read_cpsr() & 0x1F);
    printk("hcr: %b\n", read_hcr());
    assert((read_cpsr() & 0x1F) == (PE_MODE_HYP | 0b10000), "bootloader was not in Hyp mode");
    assert((read_hcr() & 0x1F) == HCR_SWIO, "HCR was not initialized");
    printk("before hvc\n");
    asm volatile ("hvc #0");
    printk("after hvc\n");
    printk("mode: %b\n", read_cpsr() & 0x1F);
}
