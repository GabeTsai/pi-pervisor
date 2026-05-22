#pragma once

static const char *ttyusb_prefixes[] = {
    "ttyUSB",	// linux
    "ttyACM",   // linux
    "cu.SLAB_USB", // mac os
    "cu.usbserial", // mac os
	0
};

#define MAX_OPEN_ATTEMPTS 5

int open_tty(const char *device);

char *find_ttyusb(void);
int set_tty_to_8n1(int fd, unsigned speed);