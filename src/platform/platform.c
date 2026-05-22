#include "platform.h"
#include "mmio.h"
#include "uart.h"


void rpi_reboot(void) {
    UART_Flush_Tx();

    PUT32(PM_WDOG, PM_PASSWORD | 1);
    PUT32(PM_RSTC, PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET);

    while (1) {
    }
}

void clean_reboot(void) {
    UART_Send_String("DONE!!!\n");
    rpi_reboot();
}
