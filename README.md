# pi-pervisor

- Bare-metal AArch32 hypervisor for the Raspberry Pi Zero 2 W / Cortex-A53, linked as a `kernel.img` at `0x8000`.
- Early boot, exception vectors, Hyp-mode entry/return, banked register handling, and lower-privilege guest entry support.
- Core hypervisor pieces for VMs, vCPUs, round-robin scheduling, idle vCPU fallback, stage-2 address translation setup, and MMIO fault dispatch.
- Guest-facing hypercall ABI covering feature discovery, guest exit/yield/advance, UART output, virtual IRQ handling, and virtual timer services.
- Low-level platform and driver support for GPIO, mini UART, BCM timers, generic timer handling, IRQ routing, MMIO helpers, barriers, and printk-style logging.
- Sample guest images and bare-metal test programs for hypercalls, exceptions, IRQs, scheduling, virtual timers, stage-2 faults, virtual UART/MMIO, and guest image loading.
