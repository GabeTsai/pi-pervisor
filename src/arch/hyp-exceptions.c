#include <stdint.h>
#include "hyp-exceptions.h"
#include "printk.h"

void hyp_dump_exception_state(uint32_t hsr, uint32_t elr_hyp, uint32_t spsr_hyp) { 
    printk("HSR: %b\n", hsr);
    printk("ELR_HYP: %p\n", elr_hyp);
    printk("SPSR_HYP: %b\n", spsr_hyp);
}