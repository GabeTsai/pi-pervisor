#pragma once 

#include <stdint.h>

__attribute__((naked))
void PUT32(uint32_t location, uint32_t value);

__attribute__((naked))
uint32_t GET32(uint32_t location);

void put32(volatile uint32_t *addr, uint32_t value);

uint32_t get32(volatile uint32_t *addr);

void RMW_OR(uint32_t reg, uint32_t mask);
void RMW_AND(uint32_t reg, uint32_t mask);

uint32_t OR32(uint32_t addr, uint32_t x);
uint32_t or32(volatile void *addr, uint32_t x);
