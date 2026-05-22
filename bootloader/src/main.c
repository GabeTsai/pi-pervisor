#include "boot-defs.h"
#include "uart.h"
#include "timer.h"
#include "mmio.h"
#include "boot-mmio.h"


void main(void) {
    UART_Init();
    boot_putuint32(GET_PROG_INFO);

    uint32_t start = TIM_SYS_Get_Ticks();
    while(1) { 
        if (UART_Has_Data()) {
            break;
        } 
        if (TIM_SYS_Get_Ticks() - start >= 300000) {
            boot_putuint32(GET_PROG_INFO);
            start = TIM_SYS_Get_Ticks();
        }
    }

    // (2) 
    // receive program info
    boot_wait_for_opcode(PUT_PROG_INFO);
    uint32_t base_addr = boot_getuint32();
    uint32_t nbytes = boot_getuint32();
    uint32_t expected_crc = boot_getuint32();
    
    // if binary collides, abort with BOOT_ERROR
    // specifically, check that sent code doesn't collide with bootloader
    // code using address of PUT32 to __prog_end__
    extern uint32_t __prog_end__[];
    uint32_t bootloader_start = (uint32_t)PUT32;
    uint32_t bootloader_end = (uint32_t)__prog_end__;

    if (!((base_addr + nbytes <= bootloader_start) || (base_addr >= bootloader_end))) {
        boot_putuint32(BAD_CODE_ADDR);
        while(1) {}
    }
    // (3) Otherwise, send GET_CODE, expected_crc back
    boot_putuint32(GET_CODE);
    boot_putuint32(expected_crc);

    // (5) Expect: PUT_CODE, <code>
    boot_wait_for_opcode(PUT_CODE);
    // read each sent byte and write it starting at <base_addr>
    for (uint32_t i = 0; i < nbytes; i++) {
        uint8_t byte = UART_Get_Char();
        PUT8(base_addr + i, byte);
    } 

    boot_putuint32(BOOT_SUCCESS);

    UART_Flush_Tx();
    
    if(!base_addr)
        rpi_reboot();

    BRANCHTO(base_addr);
    while (1) {}
}
