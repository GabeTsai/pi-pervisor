
#include "printk.h"
#include "uart.h"

static void printk_putc(char c) {
    if (c == '\n') {
        UART_Send_Char('\r');
    }
    UART_Send_Char(c);
}

static void output_num(unsigned base, uint32_t n, unsigned padding) { 
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
        printk_putc('?');
        return;
    } while (padding > (unsigned)(cur - &num[0])) { 
        printk_putc('0');
        padding--;
    }

    // buffered in reverse, so emit backwards
    while (cur > &num[0]) { 
        cur--;
        printk_putc(*cur);
    }
}

static void output_signed(int n, unsigned padding) {
    uint32_t magnitude;

    if (n < 0) {
        printk_putc('-');
        magnitude = 0u - (uint32_t)n;
        if (padding > 0) {
            padding--;
        }
    } else {
        magnitude = (uint32_t)n;
    }

    output_num(10, magnitude, padding);
}

static void output_string(const char *s) {
    if (!s) {
        s = "(null)";
    }

    while (*s) {
        printk_putc(*s++);
    }
}

int vprintk(const char *fmt, va_list args) { 
    for (; *fmt; fmt++) { 
        if (*fmt != '%') { 
            printk_putc(*fmt);
        }
        else { 
            fmt++; // skip %
            if (*fmt == '\0') {
                printk_putc('%');
                return 0;
            }

            switch (*fmt) { 
                case 'b': 
                    printk_putc('0');
                    printk_putc('b');
                    output_num(2, va_arg(args, uint32_t), 0); 
                    break;
                case 'u': output_num(10, va_arg(args, uint32_t), 0); break;
                case 'c': printk_putc(va_arg(args, int)); break;
                case 'x': output_num(16, va_arg(args, uint32_t), 0); break;
                case 'p':
                    printk_putc('0');
                    printk_putc('x');
                    output_num(16, va_arg(args, uint32_t), 0);
                    break;
                case 'd':
                    output_signed(va_arg(args, int), 0);
                    break;
                case '0':
                    fmt++;
                    int padding = 0;
                    while (*fmt >= '0' && *fmt <= '9') { 
                        padding = padding * 10 + (*fmt - '0');
                        fmt++;
                    }
                    if (*fmt == 'x') { 
                        output_num(16, va_arg(args, uint32_t), padding);
                    } else if (*fmt == 'd') { 
                        output_signed(va_arg(args, int), padding);
                    } else { 
                        printk_putc('%');
                        printk_putc('0');
                        output_num(10, padding, 0);
                        if (*fmt) {
                            printk_putc(*fmt);
                        }
                    }
                    break;
                case 's':
                    output_string(va_arg(args, char *));
                    break;
                case '%':
                    printk_putc('%');
                    break;
                default:
                    printk_putc('%');
                    printk_putc(*fmt);
                    break;
            }
        }
    }
    return 0;
}

int printk(const char *fmt, ...) { 
    va_list args;

    int ret; 
    va_start(args, fmt);
    ret = vprintk(fmt, args);
    va_end(args);
    return ret;
}
