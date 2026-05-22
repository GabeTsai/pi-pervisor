#pragma once

#include <stdint.h>

#include "uart.h"

// pi-side bootloader mmio helpers

static inline uint8_t boot_getuint8(void)
{
    return (uint8_t)UART_Get8();
}

static inline uint32_t boot_getuint32(void)
{
    uint32_t u = 0;
    for (int i = 0; i < 4; i++) {
        u |= (uint32_t)boot_getuint8() << (i * 8);
    }
    return u;
}

static inline void boot_wait_for_opcode(uint32_t expected)
{
    uint32_t window = 0;
    uint32_t nbytes = 0;

    while (window != expected || nbytes < sizeof(window)) {
        window >>= 8;
        window |= (uint32_t)boot_getuint8() << 24;
        nbytes++;
    }
}

static inline void boot_putuint32(uint32_t u)
{
    for (int i = 0; i < 4; i++) {
        UART_Put8((u >> (i * 8)) & 0xff);
    }
}
