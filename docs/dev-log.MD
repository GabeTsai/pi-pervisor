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
    - More formal Hypercall ABI version, inspired by Linux KVM and Arm SMCCC:
        - Entry:
            - r0 - hypercall number
            - r1-r3 - args 0-2
        - Return:
            - r0 - return value / error
            - r1 - r3 - optional output values 
        - r4-r11 preserved (for simplicity, we'll just use 3 args. can expand later)
        - HVC #0
    - Added skeletal guest infra: 
        - We basically do the same test as described above, but from Hyp mode, we enter EL1 (guest), set the stack from the enter_lower, then in the guest:
            - Repeatedly execute hypercalls that trap to EL2, where EL2 handles the hypercall and `ERET`s back to EL1. 
    - TODO: add actual guest main, start.S and linker script and a test for those, as well as guest binary execution

- **6/6/26**
    - Added formal guest infra in a separate directory (main, start.S, linker script that sets up stack and start/end of guest code), and a basic test that exercises the standalone guest image/load path: 
        - The test takes the guest code (`guest.bin`) and sticks it into the test image as raw bytes.
        - Then, it manually copies those bytes into the physical address where guest linked. 
        - Does the same typical EL2 to EL1 communication pathway as described above
    - Removed previous raw guest/hyp HVC API test that didn't link guest binary onto main test image. 
    - Added putchar hypercall that the guest can call 
    - Started work on virtual interrupts. 
    - First test:
        - Hyp configures timer interrupts, and sets HCR_IMO so IRQs are taken to Hyp instead of EL1. The guest code repeatedly checks IRQ count with a custom hypercall, and exits when the count reaches a certain value. 
        - All the IRQs are running on the Hyp mode's timer for now.
    - Added foundational virq infra, simple bitmask pending/active based scheduling model. see `notes.MD` for more details on the model. 
        - Hyp side - virq.h/virq.c, additional logic to route interrupts to guests,  mark them as active, pending, and done based on guest hypercalls
        - Guest side - new hypercalls to request interrupt/signal interrupt finished to hypervisor
    - Second test:
        - Exercises full guest infra again, but this time with a new guest-virq-image.bin to copy into the test image, and infra/viq-testing binary (setup irq stack through the guest linker)
        - New guest main now has a custom guest irq handler and a basic timer test
    - TODOs: 
        - harden current VIRQ Path and add more tests to cover cases like guest trying to claim a VIRQ when there are no pending ones. 
        - Create a more robust scheduler API
        - Stage 1/Stage 2 MMU experiments
        - Data-abort/fault handling
        - multi-guest with cooperative scheduling
    - STRETCH:
        - Timer-based round robin scheduling