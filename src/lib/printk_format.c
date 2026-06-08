#include <stdint.h>
#include "printk.h"

static void output_num(printk_putc_fn putc, unsigned base, uint32_t n, unsigned padding) { 
    char num[33];
    char *cur = num;

    switch (base) { 
    case 2:
        do { *cur++ = BASE2_CHARS[n % 2]; } while (n /= 2);
        break;
    case 10: 
        do { *cur++ = BASE10_CHARS[n % 10]; } while (n /= 10);
        break;
    case 16: 
        do { *cur++ = BASE16_CHARS[n % 16]; } while (n /= 16);
        break;
    default: 
        putc('?');
        return;
    } while (padding > (unsigned)(cur - &num[0])) { 
        putc('0');
        padding--;
    }

    // buffered in reverse, so emit backwards
    while (cur > &num[0]) { 
        cur--;
        putc(*cur);
    }
}

static void output_signed(printk_putc_fn putc, int n, unsigned padding) {
    uint32_t magnitude;

    if (n < 0) {
        putc('-');
        magnitude = 0u - (uint32_t)n;
        if (padding > 0) {
            padding--;
        }
    } else {
        magnitude = (uint32_t)n;
    }

    output_num(putc, 10, magnitude, padding);
}

static void output_string(printk_putc_fn putc, const char *s) {
    if (!s) {
        s = "(null)";
    }

    while (*s) {
        putc(*s++);
    }
}

int vprintk_with_putc(printk_putc_fn putc, const char *fmt, va_list args) { 
    for (; *fmt; fmt++) { 
        if (*fmt != '%') { 
            putc(*fmt);
        }
        else { 
            fmt++; // skip %
            if (*fmt == '\0') {
                putc('%');
                return 0;
            }

            switch (*fmt) { 
                case 'b': 
                    putc('0');
                    putc('b');
                    output_num(putc, 2, va_arg(args, uint32_t), 0); 
                    break;
                case 'u': output_num(putc, 10, va_arg(args, uint32_t), 0); break;
                case 'c': putc(va_arg(args, int)); break;
                case 'x': output_num(putc, 16, va_arg(args, uint32_t), 0); break;
                case 'p':
                    putc('0');
                    putc('x');
                    output_num(putc, 16, va_arg(args, uint32_t), 0);
                    break;
                case 'd':
                    output_signed(putc, va_arg(args, int), 0);
                    break;
                case '0':
                    fmt++;
                    int padding = 0;
                    while (*fmt >= '0' && *fmt <= '9') { 
                        padding = padding * 10 + (*fmt - '0');
                        fmt++;
                    }
                    if (*fmt == 'x') { 
                        output_num(putc, 16, va_arg(args, uint32_t), padding);
                    } else if (*fmt == 'd') { 
                        output_signed(putc, va_arg(args, int), padding);
                    } else { 
                        putc('%');
                        putc('0');
                        output_num(putc, 10, padding, 0);
                        if (*fmt) {
                            putc(*fmt);
                        }
                    }
                    break;
                case 's':
                    output_string(putc, va_arg(args, char *));
                    break;
                case '%':
                    putc('%');
                    break;
                default:
                    putc('%');
                    putc(*fmt);
                    break;
            }
        }
    }
    return 0;
}
