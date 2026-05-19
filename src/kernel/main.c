#include "uart.h"
#include "timer.h"

int count = 0;
__attribute__((interrupt("IRQ")))
void interrupt_vector(void) {
    count++;
    TIM_Clear_Pending();
}

void main(void) {
    UART_Init();
    TIM_Enable();
    TIM_Set_Frequency(100);
    TIM_Enable_IRQ();

    while (1) {
        UART_Send_String("Current, count: ");
        UART_Send_Int(count);
        UART_Send_String("\r\n");
        TIM_SYS_Delay_Millis(1000);
    }
}
