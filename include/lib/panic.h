#pragma once

#include "aarch32.h"
#include "printk.h"

#define PANIC_STRINGIFY(x) #x
#define PANIC_TOSTRING(x) PANIC_STRINGIFY(x)

#define panic(msg, args...) do { \
    printk("PANIC: " msg " at " __FILE__ ":" PANIC_TOSTRING(__LINE__) "\n", ##args); \
    while(1); \
} while(0)

static inline const char *trace_current_el(void) {
    uint32_t mode = read_cpsr() & CPSR_PEMODE_MASK;

    switch (mode) {
        case CPSR_PEMODE_HYP:
            return "EL2";
        case CPSR_PEMODE_USR:
            return "EL0";
        default:
            return "EL1"; 
    }
}

#define trace(args...) \
    do { printk("TRACE[%s]:%s: ", trace_current_el(), __FUNCTION__); printk(args); } while(0)

// credit to CS240LX/140e
#define die(msg...) do { 						                            \
    fprintf(stderr, ##msg);						                            \
    exit(1);                                                                \
} while(0)

// called when <syscall> fails, print <msg> and the Unix perror value and die.
// credit to CS240LX/140e
#define sys_die(syscall, msg, args...) do {                                         \
    debug("FATAL syscall error: " _XSTRING(msg) "\n\tperror reason: ", ##args);     \
    perror(_XSTRING(syscall));                                                      \
    exit(1);                                                                        \
} while(0)
