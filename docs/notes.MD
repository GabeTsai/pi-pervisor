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

### G1.4 - Execution State
- Either Aarch32 or Aarch64. 
- Exception levels use Execution States. 
- Different software layers using different ELs may be using Aarch32 or 64
#### G1.4.1 - Aarch32 PE modes
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
      - To summarize, `0x14` is the **shared** offset for hyp trap and for HVC exceptions (both synchronous) taken from a mode other than Hyp (in our case, from a mode lower because we're doing EL2/EL1/EL0)

- **AArch32 exceptions provided by EL2 - offsets and more**
   - EL2 using AArch32 provides the following exceptions that are taken to Hyp mode using the vector table entries defined in Table G1-6. Notably, amongst other exception cases:
      - **Hypervisor Call (HVC)**, or **hyper call**, which has two execution paths:
         - Taken from Hyp mode: shown explicitly in Hyp mode vector table (`0x08`)
         - If taken from any other mode: use `0x14` offset
         - A hypercall is to a hypervisor roughly what a syscall is to an OS - guest OS/code is asking the hypervisor for privileged virtualization service. Do note that the hypervisor also gets control through traps, aborts and interrupts that the guests do not explicitly request 
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

### G1.14 Asynchronous exception behavior for exceptions taken from AArch32 state
- When the PSTATE A,I,F bits are 1, the corresponding exceptions are not taken
- Masked asynchronous exceptions that are signaled remain pending unless the corresponding PSTATE bit is changed to 0
- EL2 controls routing of async exceptions with the HCR 
#### G1.14.1 Virtual exceptions when an implementation includes EL2
- EL2 virtual exceptions and corresponding physical async exceptions:
   - Virtual SError - physical external SError
   - Virtual IRQ - physical IRQ
   - Virtual FIQ - physical FIQ
- "**The HCR.{VA, VI, VF} bits generate a virtual exception only if set to 1 when the value of the corresponding HCR.{AMO, IMO, FMO} is 1.**"
   - The corresponding virtual exception is generated by setting the HCR virtual interrupt pending bit, or for Virtual IRQ/Virtual FIQ, by implementation defined mechanism (could be from interrupt controller, such as ARM Generic Interrupt Controller)
- Similar to how virtual exceptions are only taken to non-secure EL1, a virtual exception is taken only from a Non-secure EL1 or EL0 mode
   - Virtual SError to Non-secure Abort, Virtual IRQ taken to Non-secure IRQ, same for Virtual FIQ
- Corresponding mask bits in PSTATE do not mask the physical exception when HCR AMO, IMO, FMO control bits are 1. Instead, it masks the virtual exception when the PE is executing in Non-secure EL1 or EL0 mode
- Taking Virtual Abort exception clears HCR.VA to zero, Virtual IRQ or Virtual FIQ has no effect on HCR.VI/HCR.VF
- Section G1.14.2 gives more detail on the above HCR routing controls
#### Asynchronous exception masking controls
- "The HCR.{AMO, IMO, FMO} bits modify the effect of the PSTATE.{A, I, F} bits. When the value of an
HCR.{AMO, IMO, FMO} mask override bit is 1, the value of the corresponding PSTATE.{A, I, F} bit is ignored
*when the exception is taken from a Non-secure mode other than Hyp mode.*"
   - Ignored as in the CPU does not honor the current PSTATE mask bit.
   - For instance, even if the guest has interrupts masked with PSTATE.I = 1, the physical IRQ could still be taken to Hyp if the HCR.IMO = 1
   - If the exception was taken from Hyp mode, the Hyp PSTATE.I is honored and if set to 1, would lead to masking of IRQs

### G1.13 Exception return to an Exception level using AArch32
- The SPSR.IT [7:0] bits generated on exception entry are always correct for the preferred return addresses specified on page G1-4349
- **Upon an exception return, the PSTATE takes either the value loaded by the RFE instruction, or the value of the current SPSR at time of exception return for exception returns not performed by an RFE**

#### G1.13.1 Exception return instructions
- `ERET` pops up here again. Use it for retruning from an exception taken to Hyp mode. Only implemented as part of EL2
- The T bit of the value transferred to the PSTATE by an exception:
   - 0: classic ARM instruction set, A32
   - 1: T32, Thumb/Thumb-2

#### G1.13.3 Illegal return events from AArch32 state
- A return to Non-secure EL1 when AArch32 EL2 is implemented and the value of the HCR.TGE bit is 1
- Return to any mode that is not implemented
- Upon an illegal return:
   - PSTATE.IL is set to 1, and a bunch of other PSTATE bits are restored from the saved process state value (which can refer to the value in the SSPSR, value read from memory to be restored to PSTATE by an RFE, etc.)
   - PC is restored from the link address

#### G1.13.4 Legal returns that set PSTATE.IL to 1
- if the IL bit in the saved process state value is 1, then PSTATE.IL is set to 1
- idk this section didn't seem that important

#### G1.13.5 The Illegal Execution state exception
- The PSTATE.IL bit can be set by:
   - An illegal return
   - An illegal change to PSTATE.M
   - A legal return that sets PSTATE.IL to 1
- Illegal Exdcution state exception has priority over any other Undefined Instruction exception
- **On taking any exception to an Exception level that is using AArch32, PSTATE.IL bit is 1, copied to SPSR.IL bit for PE mode to which exception is taken, then the PSTATE.IL bit is cleared to 0** 
   - This also means software cannot observe the value of PSTATE.IL

### AArch32 state exception descriptions
#### G1.15.3 Hyp Trap exception
- The Hyp Trap exception is the standard mechanism for trapping Guest OS functions to they hypervisor - only implemented as part of EL2 using AArch32
- Hyp Trap exception is generated if the PE is running in Non-secure mode other than Hyp mode and commits for execution an instruction that is trapped to Hyp mode
   - You set bits to 1 in the HCR, HCPTR, HDCR or HSTR to enable isntruction traps
- Traps to Hyp mode never apply in Secure state
- ERET to retrun, use SPSR and ELR_hyp values generated by exception entry
#### G1.15.6 - Hypervisor Call (HVC) Exception
- EL2-only. 
- HVC requests a hypervisor function
- Execution of an HVC generates a Hypervisor Call exception that is taken to Hyp mode
- `ERET` for exception return, using SPSR and ELR_hyp values generated by exception entry
   - More specifically, Hyp launches lower mode by manufacturing the state that ERET consumes:ELR_hyp = next PC, SPSR_hyp = target CPSR. ERET then changes both PC and mode.
   - Lower mode returns to Hyp by taking an exception, usually HVC.
   - Hardware then fills ELR_hyp/SPSR_hyp with the return state, enters the Hyp vector, and the Hyp handler eventually ERETs back.
- "For EL2 in AArch32, executing an HVC instruction transfers immediate argument of instruction to the original HVC instruction"
- As mentioned above, the vector offset depends on the mode from which the HVC exception is taken (0x08 if taken from hyp mode, 0x14 if not)

#### G1.15.7 Prefetch Abort exception
- Can be taken to EL2/Hyp mode 
- If it is returning from Hyp mode, uses ERET instruction and SPSR_hyp and ELR_hyp values generated by exception entry
- **Prefetch Abort exception reporting a PC alignment fault exception**
   - PC alignment fault exception taken to an Exception level using AArch32 is reported as a Prefetch Abort exception. 
   - If the exception is taken to EL2 using AArch32, see G1-4385 for HSR/HIFAR and ELR_hyp bit info

#### G1.15.8 Data Abort exception
- Generated by:
   - Synchronous abort on data read/write memory access
   - SError interrupt, which might be caused by external abort on memory access (data read/write access, instruction fetch or translation table access)
- Can be taken to Hyp mode if EL2 is using AArch32
- If returning from mode other than Hyp mode, use SPSR and LR values generated by exception entry, with exception return instruction with subtraction of 8. If returning from Abort mode, this requires SPSR_abt and LR_abt
- If returning from Hyp mode, use SPSR_hyp and ELR_hyp values generated by exception entry with `ERET`
- See the **Effects of data-aborted instructions** section to see which instructions generate synchronous Data Abort exceptions, and the following rules that determine the values left in certain registers

### G1.20 Configurable instruction enables and disables, and trap controls
### G1.20.3 - EL2 configurable controls - IMPORTANT
- Table G1-35 - system regs that contain instruction enables, disables, and trap controls
- Table G1-36 - Instruction enables, disables, trap controls, for exceptions taken to Hyp mode
- Table G1-37 - Control types, for exceptions taken to Hyp mode
   - D - Disable
   - E - Enable
   - R - Routing control
   - T - Trap
- Table G1-41 - Instruction that causes exceptions when HCR.HCD is 1

### Memory ###
- Armv7 evolved the Armv6 memory model, prefetch flush renamed Instruction Synchronization Barrier
- Also, cp15 operations for barriers deprecated for dmb, dsb, and isb instructions ([source](https://developer.arm.com/community/arm-community-blogs/b/architectures-and-processors-blog/posts/memory-access-ordering-part-3---memory-access-ordering-in-the-arm-architecture))

### Hyp Mode ###
- In Hyp mode, the only exception return is execution of an ERET instruction (G1-4326)
    - PE branches to addr held in register holding preferred return addr, restores PSTATE from SPSR_<current_mode>, or SPSR_hyp in our case
    - PSTATE for Aarch32, is CPSR
- `elr_hyp` - exception link register. When taking an exception to Hyp mode, holds address to return to. 

### Registers ###
- HCR - G6-4913 - Hyp Configuration Register
   - On ARMv8/Cortex-A53, it is actually mapped to the AArch64 Sys Register HCR_EL2[31:0]. 
   - HCR.SWIO (bit 1 of HCR) means Set/Way Invalidation Override, and controls whether an EL1 cache invalidate-by-set/way operation should behave as safe clean+invalidate when virtualization is involved
      - This choice is architecturally enabled by default on Armv8-A/Cortex 53 so it's always 1
- HSR - G6-4945 - Hyp Syndrome Register
   - Holds all the info for any synchronous exception taken to Hyp mode
   - AArch64 system register ESR_EL2 is mapped to HSR
   - Provides details of the fault. 
   - Contains 6-bit Exception class field **EC** - exception cause
   - Instruction specific syndrome field ISS
   - The format of the HSR depends on the value of the EC field - see G4-4678 for more details
   - We read HSR to decode the specific cause of an exception that entered Hyp mode and route to the right hypervisor handling logic.
### Instructions ###
- ERET - F5-3117

#### ARMv8 Reference Manual
- Hyp mode - G1-4326 

#### Cortex A53 TRM


### Hypervisor Design
3 Binaries/layers:
- Bootloader: fake-OS that lives on SD card, first receiver
- Src: All the hypervisor code lives on here, and is the second receiver
- Guest: some tiny OS kernel, for testing payloads

**Linux KVM** - open-source Linux ARM hypervisor, and **Arm SMCCC** (Secure Monitor Call Calling Convention):
- KVM defines a "version" which is the rulebook that the guest and hypervisor use for hypercalls. (How should EL2 and EL1 interact when EL1 executes HVC)
   - Includes things like which registers arguments/hypercall number goes into, which reg is return value, which registers are clobbered/preserved
- ARM SMCCC explicitly use an immediate of 0 for `HVC` so we'll follow that. In general, ARM firmware ABIS also just put the function ID/args in regs and keep the immediate zero, which kkind of makes sense - the a pain to access compared to registers.  

#### Guest HVC execution path
   1. CPU takes exception to Hyp mode
   2. HVBAR selects _hyp_vector_table
   3. HVC from lower mode enters hyp_lower_sync
   4. hyp-vectors.S allocates HypExceptState on the Hyp stack
   5. hyp-vectors.S saves r0-r12, lr, HSR, ELR_hyp, SPSR_hyp, fault regs, exception_type
   6. hyp_handle_exception(hyp_state)
   7. handle_lower_sync(hyp_state)
   8. handle_hvc_from_lower(hyp_state)
   9. hyp_handle_guest_hypercall(hyp_state)
   
#### Virtual IRQ controller model
- Armv8-A supports virtual interrupts, but not necessarily a built-in generic virtual interrupt controller in the core. We still need things like interrupt IDs, priorities, masking for IRQs, etc. 
- For this repo, we use two bitmaps: one that tracks pending virqs, and another that tracks active virqs. 
- The hypervisor needs:
   - A way to record that a virtual IRQ is pending for a vCPU
   - A way to assert that there is an IRQ on the guest-visible IRQ line
   - A way to clear that IRQ line when there are no pending virtual IRQs left
- The guest needs:
   - A way to find out which virtual IRQ triggered its IRQ handler
   - A way to let the hypervisor know that it is done processing the IRQ so the appropriate bitmaps can be cleared 

#### vCPU scheduling
- Kind of like a task manager interface, but basically we need a way to switch between vCPU contexts (like thread context switching)
   - In particular, each exception-capable mode has a banked `SP` and return register except Hyp mode, which uses `ELR_hyp` and banked `SPSR`, which we will need to properly save/restore when switching between vCPU contexts
- I did a pretty simple round robin scheduler, using the ARM generic timer tick rate as the time slice
- 

#### ARM Generic Timer
- https://www.raspberrypi.com/documentation/computers/processors.html
says to use the BCM2836 Arm-local peripherals for ARM peripherals, including the generic timer. 
- The Cortex uses four central timer control and four central timer status registers. 
- Page 13 of the BCM2836 peripherals doc describes the bit offsets but the rundown is: 
   - **timer 0 = CNTPNSIRQ / non-secure EL1 physical timer**
   - timer 1 = CNTPSIRQ / secure physical timer
   - **timer 2 = CNTHPIRQ / Hyp / EL2 physical timer**
   - timer 3 = CNTVIRQ / virtual timer
- The generic timer can either run on the local APB clock, which is half the speed of the ARM, or on the crystal timer, which is always 19.2 MHz no matter what.
- If using the local APB clock, the generic timer runs at half the speed of the ARM.
- We follow what the Linux irq-bcm2836.c driver does and use the crystal timer for scheduling stability (that way changing the ARM clock doesn't mess up our timings), which gives us a tick rate/period of about 10 ms if we set the clock timer interrupt hz to 100 interrupts per second (timer condition occurs every 192000 counter ticks, or roughly 192,000 / 19,200,200 = 10 ms)
- there is no reload register in the ARM generic timer architecture, so you will have to manually rearm the timer by setting the correct number of ticks per period into the `CNTHP_TVAL` register and enabling the timer again