#include "uart.h"
#include "timer.h"
#include "printk.h"
#include "panic.h"

volatile int count = 0;

__attribute__((interrupt("IRQ")))
void interrupt_vector(void) {
    count++;
    TIM_Clear_Pending();
}

void test_func(void) { 
    trace("Current count: %d\n", count);
}

void main(void) {
    UART_Init();
    TIM_Enable();
    TIM_Set_Frequency(100);
    TIM_Enable_IRQ();

    while (1) {
        test_func();
        TIM_SYS_Delay_Millis(1000);
    }
}
