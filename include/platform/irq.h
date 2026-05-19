#pragma once

#include <stdbool.h>

#define IRQ_BASE 0x3F00B000

enum {
    IRQ_BASE_PENDING = IRQ_BASE + 0x200,
    IRQ_PENDING_1 = IRQ_BASE + 0x204,
    IRQ_PENDING_2 = IRQ_BASE + 0x208,
    IRQ_FIQ_CTRL = IRQ_BASE + 0x20C,
    IRQ_ENABLE_1 = IRQ_BASE + 0x210,
    IRQ_ENABLE_2 = IRQ_BASE + 0x214,
    IRQ_ENABLE_BASIC = IRQ_BASE + 0x218,
    IRQ_DISABLE_1 = IRQ_BASE + 0x21C,
    IRQ_DISABLE_2 = IRQ_BASE + 0x220,
    IRQ_DISABLE_BASIC = IRQ_BASE + 0x224,
};

typedef enum IRQ_Source_t {
    // peripheral sources
    IRQ_AUX_INT                     = 29,
    IRQ_I2C_SPI_SLV_INT             = 43,
    IRQ_PWA0                        = 45,
    IRQ_PWA1                        = 46,
    IRQ_SMI                         = 48,
    IRQ_GPIO_INT0                   = 49,
    IRQ_GPIO_INT1                   = 50,
    IRQ_GPIO_INT2                   = 51,
    IRQ_GPIO_INT3                   = 52,
    IRQ_I2C_INT                     = 53,
    IRQ_SPI_INT                     = 54,
    IRQ_PCM_INT                     = 55,
    IRQ_UART_INT                    = 57,

    // arm sources
    IRQ_ARM_TIMER_INT               = 0,
    IRQ_ARM_MAILBOX_INT             = 1,
    IRQ_ARM_DOORBELL_0_INT          = 2,
    IRQ_ARM_DOORBELL_1_INT          = 3,
    IRQ_GPU_0_HALTED_INT            = 4,
    IRQ_GPU_1_HALTED_INT            = 5,
    IRQ_ILLEGAL_ACCESS_0_INT        = 6,
    IRQ_ILLEGAL_ACCESS_1_INT        = 7,
    IRQ_PERIPH_1_INT                = 8,
    IRQ_PERIPH_2_INT                = 9,
    IRQ_GPU_7                       = 10,
    IRQ_GPU_9                       = 11,
    IRQ_GPU_10                      = 12,
    IRQ_GPU_18                      = 13,
    IRQ_GPU_19                      = 14,
    IRQ_GPU_53                      = 15,
    IRQ_GPU_54                      = 16,
    IRQ_GPU_55                      = 17,
    IRQ_GPU_56                      = 18,
    IRQ_GPU_57                      = 19,
    IRQ_GPU_62                      = 20,
} IRQ_Source;

bool IRQ_Check_Interrupt(IRQ_Source source);
void IRQ_Enable_Interrupt(IRQ_Source source);
void IRQ_Disable_Interrupt(IRQ_Source source);

void c_interrupt_handler(void);
