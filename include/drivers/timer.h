#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "platform.h"

// driver for the BCM legacy ARM timer plus a bit of sys timer stuff for delays

#define TIM_BASE (PERIPHERAL_BASE + 0xB000)
#define TIM_SYS_BASE (PERIPHERAL_BASE + 0x3000)

#define IRQ_EN_BIT 5
#define EN_BIT 7

enum {
    TIM_LOAD = TIM_BASE + 0x400,
    TIM_VALUE = TIM_BASE + 0x404,
    TIM_CTRL = TIM_BASE + 0x408,
    TIM_IRQ_CLR = TIM_BASE + 0x40C,
    TIM_RAW_IRQ = TIM_BASE + 0x410,
    TIM_MASK_IRQ = TIM_BASE + 0x414,
    TIM_RELOAD = TIM_BASE + 0x418,
    TIM_PREDIVIDER = TIM_BASE + 0x41C,
    TIM_FREE_RUNNING = TIM_BASE + 0x420,
};

enum TIM_CTRL_Bits {
    TIM_CTRL_FREE_RUNNING_COUNTER           = 1 << 9,

    TIM_CTRL_TIMER_HALT                     = 1 << 8,

    TIM_CTRL_TIMER_ENABLE                   = 1 << 7,

    TIM_CTRL_TIMER_INTERRUPT                = 1 << 5,

    TIM_CTRL_PRESCALE_NONE                  = 0 << 2,
    TIM_CTRL_PRESCALE_16                    = 1 << 2,
    TIM_CTRL_PRESCALE_256                   = 2 << 2,

    TIM_CTRL_16BIT_COUNTER                  = 0 << 1,
    TIM_CTRL_23BIT_COUNTER                  = 1 << 1,

    TIM_CTRL_FRC_PRESCALE_BIT               = 16
};

enum {
    TIM_SYS_CS = TIM_SYS_BASE + 0x0,
    TIM_SYS_CLO = TIM_SYS_BASE + 0x4,
    TIM_SYS_CHI = TIM_SYS_BASE + 0x8,
    TIM_SYS_C0 = TIM_SYS_BASE + 0xC,
    TIM_SYS_C1 = TIM_SYS_BASE + 0x10,
    TIM_SYS_C2 = TIM_SYS_BASE + 0x14,
    TIM_SYS_C3 = TIM_SYS_BASE + 0x18,
};

bool TIM_Check_IRQ(void);
bool TIM_Pending(void);
void TIM_Clear_Pending(void);
void TIM_Set_Frequency(uint32_t hz);
void TIM_Set_Load(uint32_t value);
void TIM_Enable(void); // also does config
void TIM_Disable(void);
void TIM_Enable_IRQ(void);
void TIM_Disable_IRQ(void);
void TIM_SYS_Delay_Millis(uint32_t ms); // this uses system clock to implement a blocking delay
uint32_t TIM_SYS_Get_Ticks(void);
void TIM_Enable_Free_Running(void);
void TIM_Delay_Nanoseconds(uint32_t nanoseconds);