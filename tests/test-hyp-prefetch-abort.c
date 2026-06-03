#include "aarch32.h"
#include "hyp-regs.h"
#include "check.h"
#include "printk.h"
#include "uart.h"

// triggers prefetch abort by fetching from bad address, 
// checks that prefetch abort hyp exception handler is taken and handles correctly
void main(void)
{
    UART_Init();
    printk("test-hyp-prefetch-abort start\n");
    printk("mode: %b\n", read_cpsr() & 0x1F);

    assert((read_cpsr() & CPSR_PEMODE_MASK) == CPSR_PEMODE_HYP, "bootloader was not in Hyp mode");
    assert((read_hcr() & 0x1F) == HCR_SWIO, "HCR was not initialized");

    printk("triggering PC alignment fault / prefetch abort\n");
    typedef void (*func_t)(void);
    func_t f = (func_t)0xDEADBEEF;
    f();
}
