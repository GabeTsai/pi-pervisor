# Pi-Pervisor

Design of pi-pervisor: a bare-metal AArch32 hypervisor for the Raspberry Pi Zero 2 W.

## 1. Hypercall / Hyp-Exception Infrastructure

```text
        EL1 GUEST                                EL2 HYPERVISOR
 ┌─────────────────────┐
 │  guest kernel code  │
 │  r0 = hcall number  │
 │  r1..r3 = args      │
 │     hvc #0          │ ◄── SMCCC-style: imm=0, args in regs
 └──────────┬──────────┘
            |
            │  exception taken to Hyp mode
            |
            |
            ▼
 ┌──────────────────────────────────────────────────────────────┐
 │ HVBAR ──► _hyp_vector_table        (hyp-vectors.S)           │
 │   0x08: HVC from Hyp     0x14: sync from lower (HVC / trap)  │
 │   0x18: IRQ              0x0C/0x10: prefetch / data abort    │
 └──────────┬───────────────────────────────────────────────────┘
            |
            │ push HypExceptState onto Hyp stack:
            │   { r0-r12, lr, HSR, ELR_hyp, SPSR_hyp,
            │     HDFAR, HIFAR, HPFAR, exception_type }
            |
            ▼
 ┌──────────────────────────────┐
 │ hyp_handle_exception()       │  dispatch on exception_type
 │   ├─ handle_lower_sync() ────┼──► decode HSR.EC
 │   │     ├─ EC=HVC ───────────┼──► hyp_handle_guest_hypercall()
 │   │     ├─ EC=WFI/WFE ───────┼──► hv_scheduler_handle_wfi()
 │   │     └─ EC=abort ─────────┼──► handle_guest_abort() → MMIO
 │   ├─ handle_irq()            │     (diagram 5)
 │   └─ handle_*_abort()        │
 └──────────┬───────────────────┘
            |
            |
            |
            │ switch (r0):  GET_ABI_VERSION │ GET_FEATURES │ EXIT
            │   YIELD │ ADVANCE │ PUTCHAR │ VIRQ_CLAIM/COMPLETE
            │   TIMER_GET_TICKS / DELAY_TICKS / GET_FREQ
            │ result ──► frame->r0
            ▼
 ┌──────────────────────────────────────────────────────────────┐
 │ HYP_ACTION_RETURN: pop HypExceptState (possibly *swapped* by │
 │ scheduler to another vCPU's frame) ► write ELR/SPSR_hyp      │
 │ ► restore r0-r12,lr ► eret  ──────────► back to EL1 guest    │
 └──────────────────────────────────────────────────────────────┘
```

## 2. Per-vCPU Saved State

```text
 ┌─ HvVcpu ───────────────────────────────────────────────────────┐
 │  id                  vCPU identifier (idle vCPU = 0xffffffff)  │
 │  vm ────────────────► owning HvVm (address space, diagram 4)   │
 │  state               IDLE │ RUNNABLE │ RUNNING │ BLOCKED │     │
 │                      EXITED                                    │
 │                                                                │
 │ ┌─ HvVcpuContext  (everything needed to resume execution) ───┐ │
 │ │                                                            │ │
 │ │ ┌─ HypExceptState  "the exception frame" ────────────────┐ │ │
 │ │ │  r0-r12, lr        general-purpose regs at trap time   │ │ │
 │ │ │  elr_hyp           guest PC to resume at               │ │ │
 │ │ │  spsr_hyp          guest CPSR (mode, A/I/F masks, T)   │ │ │
 │ │ │  hsr               syndrome of the trap that exited    │ │ │
 │ │ │  hdfar/hifar/hpfar fault addresses (VA / IPA)          │ │ │
 │ │ │  exception_type    which vector entry was taken        │ │ │
 │ │ └─────────────────────────────────────────────────────────┘ │ │
 │ │ ┌─ HypBankedRegs  (per-mode regs ERET won't restore) ─────┐ │ │
 │ │ │  sp_usr                          usr/sys mode           │ │ │
 │ │ │  sp_svc, lr_svc, spsr_svc        svc mode               │ │ │
 │ │ │  sp_irq, lr_irq, spsr_irq        irq mode               │ │ │
 │ │ └─────────────────────────────────────────────────────────┘ │ │
 │ │ ┌─ HvGuestSysRegs  (guest's view of cp15) ────────────────┐ │ │
 │ │ │  sctlr             MMU/cache enables                    │ │ │
 │ │ │  ttbr0/ttbr1/ttbcr guest stage-1 tables (future)        │ │ │
 │ │ │  dacr              domain access control                │ │ │
 │ │ │  vbar              guest exception vector base          │ │ │
 │ │ └─────────────────────────────────────────────────────────┘ │ │
 │ └──────────────────────────────────────────────────────────────┘ │
 │                                                                │
 │  virq_pending ┐  32-bit bitmaps for the virtual IRQ            │
 │  virq_active  ┘  controller (diagram 3) — per-vCPU, so IRQs    │
 │                  follow the guest, not the physical core       │
 │                                                                │
 │ ┌─ HvVcpuTimer  (the guest's private virtual timer) ─────────┐ │
 │ │  enabled │ deadline (BCM ticks) │ period (0 = one-shot)    │ │
 │ └────────────────────────────────────────────────────────────┘ │
 └────────────────────────────────────────────────────────────────┘
```

### CONTEXT SWITCH (hv_scheduler_switch_to):

1. `hv_vcpu_save(prev)` — stash prev's state: exception frame (regs, ELR, SPSR),
banked SP/LR/SPSR, guest VBAR
2. `hv_mmu_activate_vcpu(next)` — switch address space: point VTTBR at next VM's
stage-2 tables + VMID (VMID-tagged TLB entries, so no flush)
3. `hv_vcpu_load(next)` — restore next's state: banked regs, guest VBAR, and copy
its saved exception frame onto the Hyp stack
4. `hv_virq_sync(next)` — set/clear HCR.VI so next sees its own pending vIRQs

   ...then the normal exception-exit path erets into the *new* guest.

## 3. vCPU Scheduling and IRQ Handling

```text
 PHYSICAL IRQ SOURCES                EL2: handle_irq()
 ┌──────────────────┐
 │ ARM generic timer│──┐   ┌──────────────────────────────────────┐
 │ (CNTHP, EL2 tick │  ├──►│ GEN_TIM path: timeslice expired      │
 │  ~10ms @ 19.2MHz)│  │   │  hv_scheduler_advance() ► round robin│
 └──────────────────┘  │   │  GEN_TIM_rearm()  (no auto-reload)   │
 ┌──────────────────┐  │   ├──────────────────────────────────────┤
 │ BCM system timer │──┘   │ BCM path: guest virtual timers       │
 │ (guest vtimer    │      │  now = TIM_SYS_Get_Ticks()           │
 │  deadlines)      │      │  deliver_expired_timers(now)         │
 └──────────────────┘      │   └─► hv_virq_raise(vcpu,VIRQ_TIMER) │
                           │  hv_vtimer_rearm_physical()          │
                           │   (arm BCM for earliest deadline)    │
                           └────────────────┬─────────────────────┘
                                            |
                                            |
                                            |
                                            |
                                            ▼
 ┌─────────────────────────────┐   ┌─────────────────────────────┐
 │ HvScheduler                 │   │ Virtual IRQ controller      │
 │  vm[HV_MAX_VMS]             │   │  per-vCPU bitmaps:          │
 │  vcpus[guests + 1 idle]     │   │   virq_pending ──┐          │
 │  cur_idx / cur_vcpu         │   │   virq_active    │          │
 │                             │   │  hv_virq_sync(): │          │
 │  pick_next: round robin     │   │   pending? ──────┴► HCR.VI=1│
 │  no runnable ──► idle vCPU  │   │   else HCR.VI=0             │
 │  (wfi loop in EL1, parks    │   └──────────────┬──────────────┘
 │   core until next phys IRQ) │                  |  
 |                             |                  │ eret
 └─────────────────────────────┘                  ▼
 vCPU states:                       ┌─────────────────────────────┐
   IDLE → RUNNABLE ⇄ RUNNING        │ EL1 guest takes vIRQ        │
   RUNNING → BLOCKED (wfi/delay)    │ (can't tell it's virtual)   │
   RUNNING → EXITED                 │  hvc VIRQ_CLAIM ► virq id   │
 switch = hv_vcpu_save(frame) +     │  ...handle...               │
   hv_vcpu_load(next, frame)        │  hvc VIRQ_COMPLETE ► clear  │
                                    └─────────────────────────────┘
```

## 4. VM Hierarchy and Stage-2 Translation

```text
 OWNERSHIP HIERARCHY

 ┌─ HvScheduler ──────────────────────────────────────────────────┐
 │                                                                │
 │   ├── HvVm vm[2]            one VM = one guest address space   │
 │   │     ├── id, vmid        VMID tags this VM's TLB entries    │
 │   │     │                                                      │
 │   │     ├── HvVmRegion regions[8]      the "memory map" policy │
 │   │     │     { ipa_base, size, pa_base, attrs,                │
 │   │     │       type:  RAM | RAM_GUARD | MMIO,                 │
 │   │     │       device: e.g. VUART for MMIO regions }          │
 │   │     │                                                      │
 │   │     └── HvStage2 stage2            the enforcement         │
 │   │           └── L1/L2/L3 tables from a per-VM static pool    │
 │   │               (8 x 4KB pages, pool_used bump allocator)    │
 │   │                                                            │
 │   └── HvVcpu vcpus[]                                           │
 │         ├── *vm             vCPU ───► its VM                   │
 │         ├── HvVcpuContext   (frame + banked regs + sysregs)    │
 │         └── HvVcpuTimer     (virtual timer, diagram 3)         │
 │                                                                │
 └────────────────────────────────────────────────────────────────┘


 TWO-STAGE TRANSLATION

   guest VA ───(stage 1: guest's TTBR0/1 — future work)───► IPA

   "the guest thinks the IPA is a real PA"

   IPA ───(stage 2: owned by EL2, VTTBR.BADDR + VMID, VTCR)───► PA

 ┌─ stage-2 walk (long-descriptor format) ────────────────────────┐
 │                                                                │
 │   every table = one 4KB pool page, 512 x 64-bit descriptors    │
 │                                                                │
 │   L1[ IPA 31:30 ]  table desc ► L2     each entry spans 1GB    │
 │   L2[ IPA 29:21 ]  table desc ► L3     each entry spans 2MB    │
 │   L3[ IPA 20:12 ]  4KB page desc       each entry maps 4KB     │
 │                                                                │
 │   no block descriptors: L1/L2 only ever point to next-level    │
 │   tables — L3 is the only level that maps actual memory, so    │
 │   every valid translation is a full 3-level walk to a 4KB page │
 │                                                                │
 │   PA = desc[39:12] | IPA[11:0]                                 │
 │   page attrs: AF | SH | S2AP.RW | MemAttr (normal NC / device) │
 │                                                                │
 └────────────────────────────────────────────────────────────────┘

 region type ► walk outcome:

   RAM        ► mapped     ► full speed, no Hyp involvement
   RAM_GUARD  ► unmapped   ► fault ► stop guest (isolation)
   MMIO       ► unmapped *on purpose* ► fault ► device emulation
                                               (diagram 5)
```

## 5. Virtual Devices and MMIO Emulation

```text
 EL1 GUEST                              EL2 HYPERVISOR
 ┌────────────────────┐
 │ str r3, [vUART reg]│  IPA in an HV_VM_REGION_MMIO region —
 │ (thinks it's a real│  deliberately unmapped in stage 2
 │  device register)  │
 └─────────┬──────────┘
           │ stage-2 data abort ► trap to Hyp (vector 0x14)
           ▼
 ┌─────────────────────────────────────────────────────────────┐
 │ handle_guest_abort() ► build HvStage2FaultInfo              │
 │   ipa   = HPFAR[39:12] | HDFAR[11:0]                        │
 │   iss   = HSR syndrome    is_write / is_instruction         │
 └─────────┬───────────────────────────────────────────────────┘
           ▼
 ┌─────────────────────────────────────────────────────────────┐
 │ hv_mmio_handle_fault()              (src/hv/mmio.c)         │
 │  1. decode: require ISV; reject ifetch & S1 walks           │
 │     SAS ► size (1/2/4 B)   SRT ► guest register rN          │
 │  2. hv_vm_find_region(vm, ipa) ► must be type MMIO          │
 │  3. write? read rN from trapped frame (HypExceptState)      │
 └─────────┬───────────────────────────────────────────────────┘
           ▼
 ┌──────────────────────────────┐     ┌───────────────────────┐
 │ hv_virtual_device_access()   │     │ real device drivers   │
 │  region->device:             │     │                       │
 │   VUART ─► hv_vuart_write/  ─┼────►│ UART_Put8 / GET32 on  │
 │            hv_vuart_read     │     │ physical mini-UART    │
 │   (future: vGPIO, vtimer...) │     │ (policy check first)  │
 └─────────┬────────────────────┘     └───────────────────────┘
           │ read? mask/sign-extend, write result into
           │ frame->r[SRT];  advance ELR_hyp past instruction
           ▼
        eret ──► guest continues, unaware it never touched hardware
```

