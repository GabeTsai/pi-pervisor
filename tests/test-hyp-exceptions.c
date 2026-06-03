#include <stdint.h>

#include "aarch32.h"
#include "check.h"
#include "hyp-exceptions.h"
#include "hyp-regs.h"
#include "printk.h"
#include "uart.h"

#define HSR_FOR_EC(ec) ((uint32_t)(ec) << HSR_EC_SHIFT)
#define HSR_FOR_EC_ISS(ec, iss) (HSR_FOR_EC(ec) | ((iss) & HSR_ISS_MASK))

static void fill_state(HypExceptState *state, HypExceptType type, uint32_t hsr)
{
    for (uint32_t i = 0; i < 13; i++) {
        state->r[i] = 0x1000 + i;
    }

    state->lr = 0x2000;
    state->hsr = hsr;
    state->elr_hyp = 0x3000;
    state->spsr_hyp = 0x4000;
    state->hdfar = 0x5000;
    state->hifar = 0x6000;
    state->hpfar = 0x7000;
    state->exception_type = type;
}

static void run_case(const char *name, HypExceptType type, uint32_t hsr, HypExceptAction expected)
{
    HypExceptState state;

    printk("---- %s ----\n", name);
    fill_state(&state, type, hsr);
    assert(hyp_handle_exception(&state) == expected, "unexpected exception action");
}

// basic hyp exception handling tests, make sure we can handle all the exceptions 
void main(void)
{
    UART_Init();
    printk("test-hyp-exceptions start\n");
    printk("mode: %b\n", read_cpsr() & 0x1F);

    run_case("unknown", HYP_EXCEPTION_UNKNOWN,
             HSR_FOR_EC(HSR_EC_UNKNOWN),
             HYP_ACTION_HALT);

    run_case("undefined instruction", HYP_EXCEPTION_UNDEF_INSTR,
             HSR_FOR_EC(HSR_EC_UNKNOWN),
             HYP_ACTION_HALT);

    run_case("hvc from hyp", HYP_EXCEPTION_HVC_FROM_HYP,
             HSR_FOR_EC_ISS(HSR_EC_HVC_A32, 0x1234),
             HYP_ACTION_RETURN);

    run_case("prefetch abort", HYP_EXCEPTION_PREFETCH_ABORT,
             HSR_FOR_EC(HSR_EC_PREFETCH_ABORT_SAME),
             HYP_ACTION_HALT);

    run_case("data abort", HYP_EXCEPTION_DATA_ABORT,
             HSR_FOR_EC(HSR_EC_DATA_ABORT_SAME),
             HYP_ACTION_HALT);

    run_case("lower sync", HYP_EXCEPTION_LOWER_SYNC,
             HSR_FOR_EC(HSR_EC_CP15_MCR_MRC),
             HYP_ACTION_HALT);

    printk("test-hyp-exceptions done\n");
}
