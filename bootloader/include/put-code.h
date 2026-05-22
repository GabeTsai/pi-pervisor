# pragma once 

#include <stdint.h>

enum { TRACE_CONTROL_ONLY = 1, TRACE_ALL = 2 };

extern int trace_p;

void put_code(int fd, uint32_t addr, const uint8_t *buf, uint32_t n);
