#pragma once

#include <stdarg.h>

#define BASE2_CHARS "01"
#define BASE10_CHARS "0123456789"
#define BASE16_CHARS "0123456789abcdef"

typedef void (*printk_putc_fn)(char c);

int vprintk_with_putc(printk_putc_fn putc, const char *fmt, va_list args);
int vprintk(const char *fmt, va_list args);
int printk(const char *fmt, ...);
