### Memory ###
- Armv7 evolved the Armv6 memory model, prefetch flush renamed Instruction Synchronization Barrier
- Also, cp15 operations for barriers deprecated for dmb, dsb, and isb instructions ([source](https://developer.arm.com/community/arm-community-blogs/b/architectures-and-processors-blog/posts/memory-access-ordering-part-3---memory-access-ordering-in-the-arm-architecture))

### Hyp Mode ###
- In Hyp mode, the only exception return is execution of an ERET instruction (G1-4326)
    - PE branches to addr held in register holding preferred return addr, restores PSTATE from SPSR_<current_mode>, or SPSR_hyp in our case
    - PSTATE for Aarch32, is CPSR
- `elr_hyp` - exception link register. When taking an exception to Hyp mode, holds address to return to. 

### Useful Pages

#### ARMv8 Reference Manual
- Hyp mode - G1-4326 
- ERET - F5-3117

#### Cortex A53 TRM
