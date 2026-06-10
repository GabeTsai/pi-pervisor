#pragma once

#include <stdint.h>

int hv_vuart_write(uint32_t vm_id, uint32_t offset, uint32_t value, uint32_t size);
int hv_vuart_read(uint32_t vm_id, uint32_t offset, uint32_t size, uint32_t *value);
