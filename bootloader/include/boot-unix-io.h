#pragma once

#include <stdint.h>
// unix-side bootloader io helpers 

void *read_file(unsigned *size, const char *name);
void unix_putuint8(int fd, uint8_t b);
void unix_putuint32(int fd, uint32_t u);
uint8_t unix_getuint8(int fd);
uint32_t unix_getuint32(int fd);
int write_exact(int fd, const void *data, unsigned n);
