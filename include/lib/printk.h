#pragma once

#include <stdarg.h>

#define BASE2_CHARS "01"
#define BASE10_CHARS "0123456789"
#define BASE16_CHARS "0123456789abcdef"

int vprintk(const char *fmt, va_list args);
int printk(const char *fmt, ...);
