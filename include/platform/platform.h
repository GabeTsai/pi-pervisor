#pragma once

#define ARM_CLOCK_RATE 700000000
#define PERIPHERAL_BASE 0x3F000000
#define ARM_LOCAL_PERIPHERAL_BASE 0x40000000

#define PM_RSTC (PERIPHERAL_BASE + 0x10001C)
#define PM_WDOG (PERIPHERAL_BASE + 0x100024)
#define PM_PASSWORD (0x5a000000)
#define PM_RSTC_WRCFG_FULL_RESET (0x00000020)

void rpi_reboot(void) __attribute__((noreturn));
void clean_reboot(void) __attribute__((noreturn));
