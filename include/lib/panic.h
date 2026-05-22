#pragma once

#include "printk.h"

#define panic(msg, args...) do { \
    printk("PANIC: " msg " at " __FILE__ ":" __LINE__ "\n", ##args); \
    while(1); \
} while(0)

#define trace(args...) \
    do { printk("TRACE:%s: ", __FUNCTION__); printk(args); } while(0)

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
