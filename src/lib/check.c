#include "check.h"

void assert(bool condition, const char *msg) {
    if(!condition) {
        UART_Send_String("\n Assertion failed: ");
        PANIC(msg);
    }
}