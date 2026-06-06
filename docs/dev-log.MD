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
    
- **6/1/26**
    - Built more robust common exception frame/dispatcher and exception/hyp reg infra
        - More detailed hyp reg dumps, checks for correct state
    - Note: hyp trap exception handler is synthetic for now until I can get actual EL1 execution

- **6/2/26**
    - Added basic EL2 to SVC infra, appropriate case by case handling of `0x14` HVBAR offset. 
    - Tested EL2 to SVC, and SVC to Hyp properly saves  PSTATE (cspr) into spsr_hyp and regs when going to Hyp mode
    - More details on the test:
        - Start in Hyp mode
        - Use `hyp_enter_lower` to define EL1 code entry point, stack top and CPSR value to restore into lower mode
        - In EL1 code, load test value into r0, cpsr taken from hyp mode into r1. Push dummy value into r2 onto the stack, and `HVC` with a test immediate value into hyp mode. 
        - In Hyp mode, assert that HSR EC bits indicate exception from lower level mode, check that CPSR state in the lower mode is readable from the spsr_hyp, 
        and that the dummy value from r2 is readable
        - Then, return from the handler a known value back into EL1, and check that the EL1 program can read that same value. 
    - Kind of a janky test format, will cleanup asap

- **6/5/26**
    - More formal Hypercall infra, inspired by Linux KVM - open-source Linux ARM hypervisor, and Arm SMCCC (SEcure Monitor Call Calling Convention):
        - KVM defines a "version" which is the rulebook that the guest and hypervisor use for hypercalls. (How should EL2 and EL1 interact when EL1 executes HVC)
            - Includes things like which registers arguments/hypercall number goes into, which reg is return value, which registers are clobbered/preserved
        - For now, our Hypercall ABI version works like this:
            - Entry:
                - r0 - hypercall number
                - r1-r3 - args 0-2
            - Return:
                - r0 - return value / error
                - r1 - r3 - optional output values 
            - r4-r11 preserved (for simplicity, we'll just use 3 args. can expand later)
            - ARM SMCCC explicitly use an immediate of 0 for `HVC` so we'll follow that. In general, ARM firmware ABIS also just put the function ID/args in regs and keep the immediate zero, which kkind of makes sense - the a pain to access compared to registers.  
            