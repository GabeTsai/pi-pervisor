#pragma once

#include <stdint.h>

#include "platform.h"

// We assume the system clock rate is 250MHz
#define BAUD_RATE 115200
#define BAUD_RATE_REG 270
#define BASE_AUX (PERIPHERAL_BASE + 0x215000)
enum {
    AUX_ENABLES = BASE_AUX + 0x04
};

enum { 
    AUX_MU_IO_REG       = BASE_AUX + 0x40,
    AUX_MU_IER_REG       = BASE_AUX + 0x44,
    AUX_MU_IIR_REG       = BASE_AUX + 0x48,
    AUX_MU_LCR_REG       = BASE_AUX + 0x4C,   // data size is a 2 bit field. write 0b11 for 8bit
    AUX_MU_MCR_REG       = BASE_AUX + 0x50,   
    AUX_MU_LSR_REG       = BASE_AUX + 0x54,   
    AUX_MU_MSR_REG       = BASE_AUX + 0x58,
    AUX_MU_SCRATCH       = BASE_AUX + 0x5C,
    AUX_MU_CNTL_REG      = BASE_AUX + 0x60,
    AUX_MU_STAT_REG      = BASE_AUX + 0x64,
    AUX_MU_BAUD_REG      = BASE_AUX + 0x68
}; 

enum { 
    UART_TX_PIN = 14,
    UART_RX_PIN = 15
};

// initialize [XXX: we should take a baud rate?]
void UART_Init(void);
// disable
void UART_Device_Enable(void);
void UART_Enable(void);
void UART_Disable(void);

// put one byte on the uart:
// returns < 0 on error.
int UART_Put8(uint8_t c);

// returns -1 if no byte, the value otherwise.
int UART_Get8_Async(void);

// 0 = no data, 1 = at least one byte
int UART_Has_Data(void);

int UART_Tx_Is_Empty(void);
// flush out the tx fifo
void UART_Flush_Tx(void);

void UART_Send_Int(uint32_t num);
void UART_Send_String(const char *msg);
void UART_Set_Baud_Rate(uint32_t baud_reg);