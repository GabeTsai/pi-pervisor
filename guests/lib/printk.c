#include "guest_hypercall.h"
#include "printk.h"

static void guest_printk_putc(char c) {
    if (c == '\n') {
        guest_putchar('\r');
    }
    guest_putchar((uint8_t)c);
}

// guest printk internally uses putchar hypercalls to hyp for prints
int vprintk(const char *fmt, va_list args) { 
    return vprintk_with_putc(guest_printk_putc, fmt, args);
}

int printk(const char *fmt, ...) { 
    va_list args;

    int ret; 
    va_start(args, fmt);
    ret = vprintk(fmt, args);
    va_end(args);
    return ret;
}
