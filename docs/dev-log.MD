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
- Hyp entry + hypercall dispatcher
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
    - 