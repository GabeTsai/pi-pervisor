- Before: setup basic repo structure/infra
- **5/18/26** - Got a basic UART/GPIO drivers working, succesfully parked cores. 
- **5/19/26**
    - Firmware actually puts us in Hyp mode, where setting up IRQ stack is not allowed
        - Fix: modified start.S to first check if we're in Hyp mode, and switch out of it back into SVC mode with IRQ/FIQ masked
    - Setup basic exception table for interrupts
    - Got legacy BCM ARM timer working with basic interrupt test

    