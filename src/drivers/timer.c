#include "timer.h"
#include "irq.h"
#include "mmio.h"
#include "platform.h"

void TIM_Enable(void) {
    PUT32(TIM_CTRL, TIM_CTRL_TIMER_INTERRUPT | TIM_CTRL_TIMER_ENABLE | TIM_CTRL_23BIT_COUNTER);
    PUT32(TIM_PREDIVIDER, 0);
}

void TIM_Disable(void) {
    RMW_AND(TIM_CTRL, ~TIM_CTRL_TIMER_ENABLE);
    TIM_Disable_IRQ();
}


void TIM_Enable_IRQ(void) {
    IRQ_Enable_Interrupt(IRQ_ARM_TIMER_INT);
}

void TIM_Disable_IRQ(void) {
    IRQ_Disable_Interrupt(IRQ_ARM_TIMER_INT);
}

void TIM_Set_Frequency(uint32_t hz) {
    TIM_Set_Load(ARM_CLOCK_RATE / hz);
}

void TIM_Set_Load(uint32_t count) {
    PUT32(TIM_LOAD, count);
}

bool TIM_Check_IRQ(void){
    return TIM_Pending() && IRQ_Check_Interrupt(IRQ_ARM_TIMER_INT);
}

bool TIM_Pending(void){
    return GET32(TIM_MASK_IRQ) & 1;
}

void TIM_Clear_Pending(void) {
    PUT32(TIM_IRQ_CLR, 1);
}

// Do this or TIM_Enable
void TIM_Enable_Free_Running(void) {
    PUT32(TIM_CTRL, TIM_CTRL_TIMER_INTERRUPT | TIM_CTRL_TIMER_ENABLE |
                    TIM_CTRL_23BIT_COUNTER | TIM_CTRL_FREE_RUNNING_COUNTER |
                    24 << TIM_CTRL_FRC_PRESCALE_BIT);
    PUT32(TIM_PREDIVIDER, 0);
}

void TIM_SYS_Delay_Millis(uint32_t ms) {
    uint32_t start = GET32(TIM_SYS_CLO);
    uint32_t wait = ms * 1000;

    while ((GET32(TIM_SYS_CLO) - start) < wait);
}
void TIM_SYS_Delay_Micros(uint32_t us) {
    uint32_t start = GET32(TIM_SYS_CLO);
    while ((GET32(TIM_SYS_CLO) - start) < us);
}
uint32_t TIM_SYS_Get_Ticks(void) {
    return GET32(TIM_SYS_CLO);
}