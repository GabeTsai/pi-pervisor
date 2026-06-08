#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "platform.h"

#define GEN_TIM_LOCAL_CONTROL (ARM_LOCAL_PERIPHERAL_BASE + 0x00)
#define GEN_TIM_LOCAL_PRESCALER (ARM_LOCAL_PERIPHERAL_BASE + 0x08)
#define GEN_TIM_CORE0_TIMER_IRQCNTL (ARM_LOCAL_PERIPHERAL_BASE + 0x40)
#define GEN_TIM_CORE0_IRQ_SOURCE (ARM_LOCAL_PERIPHERAL_BASE + 0x60)
#define GEN_TIM_CNTHPIRQ_BIT (1 << 2) // hyp physical timer IRQ control

#define GEN_TIM_CTL_ENABLE (1 << 0)
#define GEN_TIM_CTL_IMASK (1 << 1)
#define GEN_TIM_CTL_ISTATUS (1 << 2)

#define GEN_TIM_CTL_CLK_SRC_APB (1 << 8)

// divider = 2^31 / prescaler_value. 
// with default prescale value, divider is 1.
#define GEN_TIM_DEFAULT_PRESCALER 0x80000000

void GEN_TIM_init(uint32_t hz);
void GEN_TIM_enable(void);
void GEN_TIM_disable(void);
bool GEN_TIM_irq_pending(void);
bool GEN_TIM_istatus(void);
void GEN_TIM_ack(void);
void GEN_TIM_rearm(void);
