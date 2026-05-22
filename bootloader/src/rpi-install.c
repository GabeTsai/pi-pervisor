#include "put-code.h"
#include "tty-helpers.h"
#include "boot-unix-io.h"
#include "pi-echo.h"
#include "boot-defs.h"

#include <stdio.h>

#define BAUD_RATE 115200
#define STDIN_FILENO 0

int main(int argc, char *argv[]) { 
    if (argc != 2) { 
        printf("ERROR: rpi-install: usage: %s <pi-program>\n", argv[0]);
        return 1;
    }

    char *device = find_ttyusb();
    int tty_fd = open_tty(device);
    int pi_fd = set_tty_to_8n1(tty_fd, BAUD_RATE);

    uint32_t size;
    uint8_t *code = read_file(&size, argv[1]);
    printf("read kernel from %s, size: %d\n", argv[1], size);

    put_code(pi_fd, ARMBASE, code, size);
    pi_echo(STDIN_FILENO, pi_fd, device);
}
