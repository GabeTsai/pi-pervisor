### Important Readings ###

1. `G1.2`, `G1.4`, `G1.7`  
   Exception levels, AArch32 modes, and privilege mapping.

2. `G1.8`  
   Virtualization model: Hyp mode, guest EL1/EL0, VMIDs, traps, virtual interrupts.

3. `G1.9`, `G1.10`  
   Banked registers, `SPSR_*`, `ELR_hyp`, `PSTATE`.

4. `G1.12`, `G1.13`  
   Exception vectors, exception entry, Hyp vector table, `ERET`.

5. `G1.15.3`, `G1.15.6`, then relevant abort sections  
   Hyp traps, `HVC`, prefetch/data aborts.

6. `G1.14`, `G1.20.3`  
   Interrupt routing, virtual interrupts, EL2 trap controls.

Then move to memory/VM:

7. `G3.1.2`, `G3.7`  
   Memory attributes and barriers.

8. `G4.1`, `G4.2`, `G4.3`  
   Translation regimes, disabled translation behavior, table basics.

9. `G4.5`  
   Long-descriptor tables. Key format for Hyp stage-1 and stage-2.

10. `G4.6`, `G4.7`  
   Permissions and memory attributes.

11. `G4.8`, `G4.9`  
   TLB behavior and maintenance.

12. `G4.11`, `G4.12`  
   Memory aborts and Hyp syndrome/fault reporting.

13. `G3.4`, `G4.10`, `G4.13`, `G4.17`  
   Caches, address translation helpers, and register group summaries.

Don’t jump into stage-2 tables before you understand `G1.8`, `G1.12`, and `G1.13`. For a base hypervisor, exception entry/return and Hyp-mode trap reporting are the spine.


### Exception Levels - G1.2 ###
- On taking an exception, EL only increases or remains the same
- On returning, EL only decreases or remains the same
- The *target Exception level* is the Exception level that execution changes to /remains in
on taking an or returning from an exception
- An exception cannot target EL0 
- At the current EL, the PE can access resources availble for current EL and current sec state, as well as resources available at lower EL levels (provided the current security state allows access to those resources)
- A Hypervisor typically sits at the **EL2** level

#### G1.4 - Execution State
- Either Aarch32 or Aarch64. 
- Exception levels use Execution States. 
- Different software layers using different ELs may be using Aarch32 or 64
##### G1.4.1 - Aarch32 PE modes
- Aarch32 provides a set of PE modes that support normal software execution/exception handling
- G1-4327 describes the set of registers available in each PE mode
- Modes:
    - Monitor - EL3
    - Hyp mode - EL2
    - Sys, Supervisor, Abort, Undefined, IRQ, FIQ - EL depends on security state
    - User - EL0
- The mode in use right before an exception taken - mode exception is *taken from*
- Mode used on taking the exception - mode the exception is *taken to*
    - Ex: we take an exception from mode EL2 to mode EL3
- EL2/EL3 config settings can affect a common condition like interrupt routing, otherwise when no traps are enabled, the mode an exception is taken to is the *default mode* for that exception

### G1.7 - Security State, Exception levels, AArch32 execution privilege
- EL2 is never implemented in Secure state, EL3 is never implemented in Non-secure state
- In AArch 32, within a given Security state, the current PE mode indicates execution privleges, current PE state
- Hyp mode is only implemented in Non-secure state, and only if EL2 is using AArch 32
- Since we aren't doing any EL3 stuff, Sys, FIQ, IRQ, Supervisor, Abort and Undefined modes are implemented in Non-secure (since EL1 is using Aarch32)
- "user mode is implemented if EL0 is using AArch32"
#### G1.7.1 - Limited use of Privilege level in ARMv8 AArch32 state
- AArch32 uses the VMSAv8-32 translation regime

### G1.8 - Virtualizaiton
- We use an `ERET` instruction to return to software running at EL1 or EL0 from hyp mode (EL2)
#### The Basic Components of a Virtualized System 
   1. Hypervisor running in EL2 - responsible for switching between *virtual machines*. 
      - Virtual machines contain EL1 and EL0 PE modes
   2. Some number of guest OSs that run in Non-secure EL1 on a virtual machine
   3. For each guest OS, applications that usually run in Non-secure EL0 on a virtual machine \
- From highest to lowest privilege: 
   - Hypervisor (EL2) -> Guest OS Kernel on VM N (EL1) -> Guest OS Kernel's programs on VM N (EL0)

- Armv8 provides the option for Guest OSs to be aware/unaware that it is running on a virtual machine

- Hypervisor assigns a **Virtual Machine Identifier(VMID)** to each virtual machine
- EL2 provides various controls to support Guest OS management: 
   - Virtual values for small number of ID registers - read of one of these regs by Guest OS always returns virtual value
   - Trap various mem management operations/accesses to many other registers that generate an exception taken to EL2
   - route interrupts to current Guest OS, Guest OS currently not running, or they hypervisor

- In Non-Secure state:
   - Independent translation regime for memory accesses from EL2
   - PL1/0 translation regime uses two-stage address translation (more details in VM sys arch section):
      - Stage 1: VA to *intermediate physical address* (IPA) - managed at EL1 by Guest OS
         - The Guest OS believes the IPA is the actual PA
      - Stage 2: Map IPA to PA - managed at EL2. Guest OS could have no idea this is happening
   - Basically, the Guest OS works with VA to IPA translation (for our purposes let's say they think they just translated to a PA), and then the hypervisor takes the IPA and converts it to a PA

#### G1.8.1 EL2 Exception Model
- The following exceptions are required for an EL2 implementation: 
   - **Hypervisor Call (HVC) exception** - we're going to see this come up a lot. 
      - Always taken to EL2. 
   - Traps to EL2 (see G1-4427)
   - All **virtual interrupts**: Virtual SError, Virtual IRQ/FIQ. 
      - Always taken to EL1, can only be taken from Non-secure EL1 or EL0
      - When a VI is enabled, its corresponding physical exception is taken to EL2 (unless EL3 config'd)
      - Software executing at EL2 use VIs to signal physical interrupts to Non-secure EL1 and Non-secure EL0. 
      - Ex G1-1 on G1-4321 shows a useful example of such:
         1. EL2 software routes physical interrupt to EL2
         2. Exception handler in EL2 determines whether the interrupt can be handled in EL2 or requires routing to guest OS in EL1. In the case of routing, the hypervisor uses the appropriate virtual interrupt type to signal the physical interrupt to the currently running guest OS, or marks it as pending for a not currently running guest OS so the guest OS can be appropriately signaled later when it wakes up
      - **Non-secure EL1/EL0 modes can't tell a virtual interrupt and its corresponding physical interrupt apart**
      - **Purpose of virtual interrupts:**
         - The EL2 hypervisor controls all physical interrupt routing so it can decide which events are handled by EL2 versus guest OS's
         - EL2 uses virtual interrupts to allow guests to access selected hardware or emulated-device events
            - The guest handles the virtual interrupt as if it owned interrupt delivery
         - Interrupts generated by one guest should only go to that guest, and none other. 
            - Consequences if this doesn't happen: guests interfering with hardware they shouldn't be able to access, spurious interrupts when the guest isn't running/didn't generate them, observation of events meant for another virtual machine

   - Controls that can be used to route some synchronous exceptions taken from Non-secure to EL2
   - Provide ways to trap PE operations to EL2
      - For ops trapped to EL2, the hypervisor usually emulates the required operation (Guest OS unaware), or returns an error to the Guest OS

### G1.12 - Handling exceptions that are taken to an Exception level using AArch32 - IMPORTANT SECTION!!!
#### G1.12.1 Exception vectors and the exception base address
- AArch32 still uses an exception vector table like Armv6
   - Defines exception vector tables for exceptions taken to EL2 and EL3
   - 8 vectors form the AArch32 word-aligned vector table 
- If you're in EL2:
   - In addition to the base exception table, you must include an exception table for exceptions taken to Hyp mode, which forms the **Hyp vector table**. 
   - HVBAR holds the exception base address of this vector table
- See G1-6 on G1-4342 for the AArch32 vector table entries
   - The Hyp column defines the vector table used when an exception is taken to EL2
   - Of particular note is `0x14` offset entry: Hyp Trap/Hyp mode entry, which is used for exceptions that cause entry to Hyp mode
      - This entry basically tells you which address you enter Hyp mode (the reason why is handled by other registers)
      - This is never used for IRQ/Virtual IRQ, FIQ/Virtual FIQ exceptions (G1-4343)
      - It is shared! guest HVC, Hyp traps, and more can all enter at HVBAR + `0x14`

- **AArch32 exceptions provided by EL2 - offsets and more**
   - EL2 using AArch32 provides the following exceptions that are taken to Hyp mode using the vector table entries defined in Table G1-6. Notably, amongst other exception cases:
      - **Hypervisor Call (HVC)**, or **hyper call**, which has two execution paths:
         - Taken from Hyp mode: shown explicitly in Hyp mode vector table (`0x08`)
         - If taken from any other mode: use `0x14` offset
      - **Hyp Trap** - always uses offset `0x14`

#### G1.12.2 Exception prioritization for exceptions taken to AArch32 state
- Laundry list of synchronous exception priority. Most important for the hypervisor: 
   1. HVC is relatively low priority. Getting prefetch aborts or undefined instruction exceptions instead is architecturally expected. 
   2. Executing a HVC instruction when HVC instructions are disabled is an Undefined instruction
   3. Lots of configurable traps like HSTR.Tn, HCR.TIDCP, etc. rank higher than a HVC
   4. Toggling HCR (Hyp Configuration Register) Trap General Exceptions (TGE) allows several exceptions normally handled by the guest OS at EL1 to be routed to Hyp/EL2. For first implementations, better to keep this off

#### G1.12.3 Overview of Exception Entry
- Upon entering an exception, ARMv8 saves a link value based on the preferred return address for that exception for use upon return from the section. 
   - For Hyp Trap - address of trapped instruction
   - For HVC - address of instruction AFTER the HVC instruction
- For exceptions taken to a non-EL2 mode, the link value is saved in the LR for the mode to which the exception is taken. Table G1-10 shows the related link offsets. 
- For exceptions taken to an EL2 mode, the link value is saved in the `ELR_hyp` register - the specific value is defined in Table G1-9 on G1-4349, with no offset (hence we do eret instead of the classic `lr - 4` or `lr - 8`)

#### G1.12.4 PE mode for taking exceptions
- For EL2 implementations:
   - An exception taken from Non-secure that is also not taken to Secure Monitor is taken to Non-secure state.
   - Also:
      - "If the exception is taken from Hyp mode, then it is taken to Hyp mode" (G1-4351)
      - Otherwise, the exception is taken to Hyp mode as described in G1-4352 or taken to the default mode for the exception
      - **Exceptions taken to Hyp mode** - G1-4352:
         - Any exception taken from Hyp mode (not routed to EL3) is taken to Hyp mode
         - Exceptions taken from a Non-secure state that are also taken to Hyp mode:
            - Abort that should taken to Hyp mode (G4-4649 defines such routings)
            - Hyp Trap exception
            - HVC/Hypervisor Call exception
            - SError interrupt, IRQ or FIQ exception explicitly routed to Hyp mode
            - Synchronous external abort, Alignment fault, Undef Instr exception, Supervisor Call taken from EL0 and explicitly routed to Hyp mode (so basically, a bunch of guest code exceptions)
            - Debug exception explicitly ourted to Hyp mode
            - **Virtual exceptions are always taken to a Non-secure EL1 mode!!!**
- See page G1-4376 for a subsection that describes the modes and offsets to which each exception can be taken

#### G1.12.5 PE state on exception entry
- Describes instruction set stateon entry
- PSTATE.E - load/store endianness for data handling. For Hyp mode, HSCTLR.EE contains the appropriate bit
- For Hyp modes, SCR.EA, IRQ, and FIQ determine PSTATE.A, I, and F

#### G1.12.6 Routing exceptions from Non-secure EL0 to EL2
- This section describes handling/reporting of exceptions that are taken to EL2 when HCR.TGE is 1 - basically, setting that bit is for a model where Non-secure EL0 is controlled directly by EL2 - not relevant for us

#### G1.12.7 Routing debug excptions to EL2 using AArch32
- Not implementing this for now...

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
