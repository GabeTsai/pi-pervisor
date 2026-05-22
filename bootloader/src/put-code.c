#include "put-code.h"
#include "boot-crc.h"
#include "boot-defs.h"
#include "boot-unix-io.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int trace_p = 0;

static inline uint8_t trace_get8(int fd)
{
    uint8_t v = unix_getuint8(fd);
    if (trace_p == TRACE_ALL) {
        printf("GET8:%x\n", v);
    }
    return v;
}

static inline uint32_t trace_get32(int fd)
{
    uint32_t v = unix_getuint32(fd);
    if (trace_p) {
        printf("GET32:%x [%s]\n", v, boot_op_to_str(v));
    }
    return v;
}

static inline void trace_put8(int fd, uint8_t v)
{
    if (trace_p == TRACE_ALL) {
        printf("PUT8:%x\n", v);
    }
    unix_putuint8(fd, v);
}

static inline void trace_put32(int fd, uint32_t v)
{
    if (trace_p) {
        printf("PUT32:%x [%s]\n", v, boot_op_to_str(v));
    }
    unix_putuint32(fd, v);
}

static inline uint32_t get_op(int fd)
{
    // we do not trace the output from PRINT_STRING so do not call the
    // tracing operations here except for the first word after we are 
    // sure it is not a <PRINT_STRING>
    while(1) {
        uint32_t op = unix_getuint32(fd);
        if(op != PRINT_STRING) {
            if(trace_p)
                printf("GET32:%x [%s]\n", op, boot_op_to_str(op));
            return op;
        }
        if (op == BOOT_ERROR) { 
            perror("rpi-install: pi reported boot error\n");
            exit(1);
        }
    }
}

void put_code(int fd, uint32_t addr, const uint8_t *buf, uint32_t n)
{
    printf("simple_boot: waiting for Pi bootloader\n");
    uint32_t word;
    while((word = get_op(fd)) != GET_PROG_INFO) { 
        trace_get8(fd);
    }
    printf("simple_boot: sending %d bytes, crc32=%x\n", n, crc32(buf,n));
    trace_put32(fd, PUT_PROG_INFO);
    usleep(300000);
    trace_put32(fd, addr);
    trace_put32(fd, n);
    trace_put32(fd, crc32(buf, n));

    // (4)
    // clear garbage again
    while((word = get_op(fd)) == GET_PROG_INFO) {}
    if (word != GET_CODE) {
        fprintf(stderr, "rpi-install: expected GET_CODE, got %x [%s]\n",
                word, boot_op_to_str(word));
        exit(1);
    }

    // check crc from pi is the crc we sent
    uint32_t pi_crc = trace_get32(fd);
    if (pi_crc != crc32(buf, n)) {
        fprintf(stderr, "rpi-install: pi crc (%x) != expected crc (%x)\n",
                pi_crc, crc32(buf, n));
        exit(1);
    }

    trace_put32(fd, PUT_CODE);
    // for each byte in code, send it to pi
    for (uint32_t i = 0; i < n; i++) {
        trace_put8(fd, buf[i]);
    }

    word = trace_get32(fd);
    if (word != BOOT_SUCCESS) {
        fprintf(stderr, "rpi-install: pi reported boot failure\n");
        exit(1);
    }

    printf("rpi-install: boot successful\n");
}
