#include "generic_timer.h"

#include "aarch32.h"
#include "check.h"
#include "mmio.h"
#include "printk.h"


static uint32_t gen_tim_period_ticks;

// CNTFRQ is the frequency of the system counter
static inline uint32_t read_cntfrq(void) {
    uint32_t value;
    asm volatile ("mrc p15, 0, %0, c14, c0, 0" : "=r"(value));
    return value;
}

static inline uint32_t read_cnthp_tval(void) {
    uint32_t value;
    asm volatile ("mrc p15, 4, %0, c14, c2, 0" : "=r"(value));
    return value;
}

static inline void write_cnthp_tval(uint32_t value) {
    asm volatile ("mcr p15, 4, %0, c14, c2, 0" : : "r"(value));
    ISB();
}

// CNTHP_CTL is the control register for the Hyp mode physical timer.
static inline uint32_t read_cnthp_ctl(void) {
    uint32_t value;
    asm volatile ("mrc p15, 4, %0, c14, c2, 1" : "=r"(value));
    return value;
}

static inline void write_cnthp_ctl(uint32_t value) {
    asm volatile ("mcr p15, 4, %0, c14, c2, 1" : : "r"(value));
    ISB();
}

static void init_local_counter_frequency(void) {
    DSB();
    PUT32(GEN_TIM_LOCAL_CONTROL, 0);
    // technically don't need to do this with crystal timer but good to be explicit
    PUT32(GEN_TIM_LOCAL_PRESCALER, GEN_TIM_DEFAULT_PRESCALER);
    DSB();
    ISB();
}

// turn on the IRQ for hyp physical timer
static void enable_local_cnthp_irq(void) {
    DSB();
    PUT32(GEN_TIM_CORE0_TIMER_IRQCNTL,
          GET32(GEN_TIM_CORE0_TIMER_IRQCNTL) | GEN_TIM_CNTHPIRQ_BIT);
    DSB();
}

static void disable_local_cnthp_irq(void) {
    DSB();
    PUT32(GEN_TIM_CORE0_TIMER_IRQCNTL,
          GET32(GEN_TIM_CORE0_TIMER_IRQCNTL) & ~GEN_TIM_CNTHPIRQ_BIT);
    DSB();
}

void GEN_TIM_init(uint32_t hz) {
    uint32_t cntfrq;

    assert(hz != 0, "generic timer frequency cannot be zero");

    init_local_counter_frequency();

    cntfrq = read_cntfrq();
    assert(cntfrq != 0, "generic timer CNTFRQ is zero");

    gen_tim_period_ticks = cntfrq / hz; // set the period of timer in ticks

    write_cnthp_ctl(GEN_TIM_CTL_IMASK);
    disable_local_cnthp_irq();
}

void GEN_TIM_enable(void) {
    assert(gen_tim_period_ticks != 0, "generic timer not initialized");

    enable_local_cnthp_irq();
    write_cnthp_tval(gen_tim_period_ticks);
    write_cnthp_ctl(GEN_TIM_CTL_ENABLE);
}

void GEN_TIM_disable(void) {
    write_cnthp_ctl(GEN_TIM_CTL_IMASK);
    disable_local_cnthp_irq();
}

bool GEN_TIM_irq_pending(void) {
    if (!GEN_TIM_istatus()) { 
        return false;
    }

    DSB();
    return (GET32(GEN_TIM_CORE0_IRQ_SOURCE) & GEN_TIM_CNTHPIRQ_BIT) != 0;
}

// ISTATUS indicates whether the timer condition is met 
// (did we count down all the way to zero)
bool GEN_TIM_istatus(void) {
    return (read_cnthp_ctl() & GEN_TIM_CTL_ISTATUS) != 0;
}

void GEN_TIM_ack(void) {
    uint32_t ctl = read_cnthp_ctl();
    write_cnthp_ctl((ctl & GEN_TIM_CTL_ENABLE) | GEN_TIM_CTL_IMASK);
}

void GEN_TIM_rearm(void) {
    assert(gen_tim_period_ticks != 0, "generic timer not initialized");

    write_cnthp_tval(gen_tim_period_ticks);
    write_cnthp_ctl(GEN_TIM_CTL_ENABLE);
}
