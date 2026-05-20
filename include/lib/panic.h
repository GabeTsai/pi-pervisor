#pragma once

#include "printk.h"

#define panic(msg, args...) do { \
    printk("PANIC: " msg " at " __FILE__ ":" __LINE__ "\n", ##args); \
    while(1); \
} while(0)

#define trace(args...) \
    do { printk("TRACE:%s: ", __FUNCTION__); printk(args); } while(0)
