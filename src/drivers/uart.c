#include "uart.h"
#include "gpio.h"
#include "aarch32.h"
#include "mmio.h"

static PinT uart_tx_pin = {UART_TX_PIN};
static PinT uart_rx_pin = {UART_RX_PIN};

void UART_Init(void) { 
    DSB();

    // GPIO pins should be set up before enabling UART
    GPIO_set_mode(&uart_tx_pin, ALT5);
    GPIO_set_mode(&uart_rx_pin, ALT5);

    // Set both GPIO pins to high 
    // GPIO_set_high(&uart_tx_pin);
    // GPIO_set_high(&uart_rx_pin);
    
    // we're writing to a different device now so we need barrier
    DSB();
    UART_Device_Enable();
    DSB();
    
    PUT32(AUX_MU_CNTL_REG, 0x00);   // Disable TX/RX 
    PUT32(AUX_MU_IER_REG, 0x00); // Disable interrupts
    PUT32(AUX_MU_IIR_REG, 0b110); // clear FIFOs
    PUT32(AUX_MU_LCR_REG, 0b11);    // Set data size to 8 bits
    PUT32(AUX_MU_MCR_REG, 0x00);
    PUT32(AUX_MU_BAUD_REG, BAUD_RATE_REG);  // Set baud rate
    UART_Enable();
    DSB();
}

void UART_Enable(void) { 
    PUT32(AUX_MU_CNTL_REG, 0b11); // enable TX and RX
}

void UART_Device_Enable(void) { 
    uint32_t enables = GET32(AUX_ENABLES);
    enables |= 0b1; // set bit 0 to enable mini UART
    PUT32(AUX_ENABLES, enables);
}

void UART_Disable(void) { 
    DSB();
    UART_Flush_Tx();
    PUT32(AUX_MU_CNTL_REG, 0x00); // disable mini UART
    // rmw to aux enables
    uint32_t enables = GET32(AUX_ENABLES);
    enables &= !0b1; // clear bit 0 to disable mini UART
    PUT32(AUX_ENABLES, enables);
    DSB();
}

int UART_Get8(void) { 
    DSB();
    // check bit 5 of LSR
    while ((GET32(AUX_MU_LSR_REG) & 1) == 0) {
        // wait for data to be available
    }
    int res = (int)(GET32(AUX_MU_IO_REG) & 0xFF);
    DSB();
    return res;
}

int UART_Put8(uint8_t c) { 
    DSB();
    while ((GET32(AUX_MU_LSR_REG) & (0b100000)) == 0) {
        // wait for data to be available
    }
    PUT32(AUX_MU_IO_REG, c);
    DSB();
    return 1;
}

// returns -1 if no byte, the value otherwise.
int UART_Get8_Async(void) { 
    DSB();
    if(!UART_Has_Data())
        return -1;
    return UART_Get8();
}

// 0 = no data, 1 = at least one byte
// bit 0 is set if the receive FIFO contains at least 1 symbol
int UART_Has_Data(void) { 
    DSB();
    int res = (GET32(AUX_MU_LSR_REG) & 0x01) != 0;  
    DSB();
    return res;
}

int UART_Tx_Is_Empty(void) { 
    DSB();
    // bit 6 (7th bit) of LSR is set if TX FIFO is empty and idle
    int res = (GET32(AUX_MU_LSR_REG) & (0b1000000)) != 0;
    DSB();
    return res;
}

// flush out the tx fifo
void UART_Flush_Tx(void) { 
    while(!UART_Tx_Is_Empty()){}
}

void UART_Send_Int(uint32_t num) {
    if (num == 0) {
        UART_Put8('0');
    }
    char output[11] = "\0\0\0\0\0\0\0\0\0\0";
    uint8_t i = 0;
    while (num > 0 && i < sizeof(output)) {
        output[i] = '0' + num % 10;
        num = num / 10;
        i++;
    }
    
    for (char *c = output + i - 1; c >= output; c--) {
        if (*c) {
            UART_Put8(*c);
        }
    }
}

void UART_Send_String(const char *msg) {
    while (*msg) {
        UART_Put8(*msg);
        msg++;
    }
}

void UART_Send_Hex(uint32_t num) {
    UART_Send_String("0x");
    for (int8_t i = 28; i >= 0; i-= 4) {
        uint8_t number = num >> i & 0xF;
        if (number < 10) {
            UART_Put8('0' + number);
        }
        else {
            UART_Put8('A' + number - 10);
        }
    }
}

void UART_Set_Baud_Rate(uint32_t baud_reg) {
    DSB();
    PUT32(AUX_MU_CNTL_REG, 0); // disable TX/RX while changing baud rate
    PUT32(AUX_MU_BAUD_REG, baud_reg);
    PUT32(AUX_MU_CNTL_REG, 0b11); // re-enable 
    DSB();
}
