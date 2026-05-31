## Todos: 
### Base Setup
- bootloader + repeatable install path - DONE
- exception trampolines
- panic/assert dumps - DONE
- trace flags + MMIO trace - DONE
- mailbox
- cycle counter / PMU basics
- ARM generic timer
- GPIO interrupts
- kmalloc + debug allocator (memcheck)
- SWI syscall harness 

### Early Hyp Infra
### MMU
### Multi-Core

## Dev Log
- Before: setup basic repo structure/infra
- **5/18/26** - Got a basic UART/GPIO drivers working, succesfully parked cores. 
- **5/19/26**
    - Firmware actually puts us in Hyp mode, where setting up IRQ stack is not allowed
        - Fix: modified start.S to first check if we're in Hyp mode, and switch out of it back into SVC mode with IRQ/FIQ masked
    - Setup basic exception table for interrupts
    - Got legacy BCM ARM timer working with basic interrupt test

- **5/22/26** 
    - Ported 140E bootloader over to PiZero2W, can now `rpi-install` .bin programs
        - Weird bug where there was a junk byte in the UART RX when the host was sending the first `PUT_PROG_INFO` 
        byte in the bootloader protocol, so first 4 bytes read was junk byte + the first 3 bytes of the `PUT_PROG_INFO` opcode
        - Fix: Pi loader keesp scanning incoming byte stream until it sees the full `PUT_PROG_INFO` opcode from the host, then continues the 
        boot protocol
        - Could also flush RX as fix 

- **5/23/26 - 5/27/26**
    - Read up on all the hypervisor infrastructure, took a bunch of notes

- **5/31/26**
    - Modified bootloader and `src/boot/start.S`to boot in Hyp mode, confirmed with basic test
    - Installed basic hyp exception vector table and cleared HCR
    - Confirmed basic HVC works and is handled properly by HVC handler
    - TODO: build common exception frame/dispatcher