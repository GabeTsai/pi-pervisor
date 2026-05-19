#include "irq.h"
#include "mmio.h"

bool IRQ_Check_Interrupt(IRQ_Source source) {
    if (source >= IRQ_AUX_INT) { 
        uint32_t pending_reg = source < 32 ? IRQ_PENDING_1 : IRQ_PENDING_2;
        return (GET32(pending_reg) >> (source % 32)) & 1;
    }
    else {
        return (GET32(IRQ_BASE_PENDING) >> source) & 1;
    }
}

void IRQ_Enable_Interrupt(IRQ_Source source) {
    if (source >= IRQ_AUX_INT) {
        uint32_t enable_reg = source < 32 ? IRQ_ENABLE_1 : IRQ_ENABLE_2;
        PUT32(enable_reg, 1 << (source % 32));
    }
    else {
        PUT32(IRQ_ENABLE_BASIC, 1 << source);
    }
}

void IRQ_Disable_Interrupt(IRQ_Source source) {
    if (source >= IRQ_AUX_INT) {
        uint32_t disable_reg = source < 32 ? IRQ_DISABLE_1 : IRQ_DISABLE_2;
        PUT32(disable_reg, 1 << (source % 32));
    }
    else {
        PUT32(IRQ_DISABLE_BASIC, 1 << source);
    }
}

// weak definitions for all the vectors to be overwritten in user code

void __attribute__((interrupt("UNDEF"), weak)) undefined_instruction_vector(void) {
    while (1);
}

void __attribute__((interrupt("SWI"), weak)) software_interrupt_vector(void) {
    return;
}

void __attribute__((interrupt("ABORT"), weak)) prefetch_abort_vector(void) {
    while (1);
}

void __attribute__((interrupt("ABORT"), weak)) data_abort_vector(void) {
    while (1);
}

void  __attribute__((interrupt("IRQ"), weak)) interrupt_vector(void) {
    return;
}

void __attribute__((interrupt("FIQ"), weak)) fast_interrupt_vector(void) {
    return;
}

void __attribute__((weak)) c_interrupt_handler() {
    return;
}
