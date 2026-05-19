#pragma once

#include "uart.h"
#include <stdbool.h>

#define PANIC(msg) do { \
    UART_Send_String("PANIC: "); \
    UART_Send_String(msg); \
    UART_Send_String(" at "); \
    UART_Send_String(__FILE__); \
    UART_Send_String(":"); \
    UART_Send_Int(__LINE__); \
    UART_Send_String("\n"); \
    while(1); \
} while(0)


void assert(bool condition, const char *msg);
#define ASSERT(condition, msg) assert(condition, msg)

