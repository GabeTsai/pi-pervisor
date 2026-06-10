ARMGNU ?= arm-none-eabi
ARMCPU ?= cortex-a53

CC := $(ARMGNU)-gcc
OBJCOPY := $(ARMGNU)-objcopy
OBJDUMP := $(ARMGNU)-objdump

BUILD_DIR := build
GUEST_SCHED_IDS := 0 1
GUEST_SCHED_BASE_0 := 0x20000
GUEST_SCHED_BASE_1 := 0x20000
GUEST_SCHED_TARGETS := $(GUEST_SCHED_IDS:%=guest-sched%)
GUEST_TIMER_IDS := 0 1
GUEST_TIMER_BASE_0 := 0x20000
GUEST_TIMER_BASE_1 := 0x20000
GUEST_TIMER_TARGETS := $(GUEST_TIMER_IDS:%=guest-timer%)
GUEST_MMIO_UART_IDS := 0 1
GUEST_MMIO_UART_BASE_0 := 0x20000
GUEST_MMIO_UART_BASE_1 := 0x20000
GUEST_MMIO_UART_TARGETS := $(GUEST_MMIO_UART_IDS:%=guest-mmio-uart%)
TARGETS := guest guest-virq $(GUEST_SCHED_TARGETS) $(GUEST_TIMER_TARGETS) $(GUEST_MMIO_UART_TARGETS)

COMMON_S_SRCS := start.S guest-vectors.S lib/guest_hypercall.S
COMMON_OBJS := $(patsubst %.S,$(BUILD_DIR)/%.o,$(COMMON_S_SRCS))
COMMON_OBJS += $(BUILD_DIR)/lib/printk.o $(BUILD_DIR)/repo/src/lib/printk_format.o
GUEST_OBJS := $(COMMON_OBJS) $(BUILD_DIR)/main.o
GUEST_VIRQ_OBJS := $(COMMON_OBJS) $(BUILD_DIR)/virq-main.o
GUEST_STAGE2_FAULT_OBJS := $(COMMON_OBJS) $(BUILD_DIR)/stage2-fault-main.o
DEPS := $(GUEST_OBJS:.o=.d) $(GUEST_VIRQ_OBJS:.o=.d) $(GUEST_STAGE2_FAULT_OBJS:.o=.d)

CPPFLAGS := -I../include -I../include/arch -I../include/hv -I../include/lib -Ilib -MMD -MP
CFLAGS := -O2 -Wall -Wextra -Werror -ffreestanding -nostdlib -nostartfiles
CFLAGS += -std=gnu99 -mcpu=$(ARMCPU) -marm -mno-unaligned-access
LDFLAGS := -T linker.ld -nostdlib -nostartfiles -ffreestanding

.PHONY: all clean

all: $(TARGETS:%=%.bin)

guest.bin: $(BUILD_DIR)/guest.elf
	$(OBJCOPY) $< -O binary $@

guest-virq.bin: $(BUILD_DIR)/guest-virq.elf
	$(OBJCOPY) $< -O binary $@

guest-stage2-fault.bin: $(BUILD_DIR)/guest-stage2-fault.elf
	$(OBJCOPY) $< -O binary $@

$(BUILD_DIR)/guest.elf: $(GUEST_OBJS) linker.ld Makefile
	$(CC) $(CFLAGS) $(GUEST_OBJS) $(LDFLAGS) -Wl,-Map=$(BUILD_DIR)/guest.map -o $@
	$(OBJDUMP) -D $@ > $(BUILD_DIR)/guest.list

$(BUILD_DIR)/guest-virq.elf: $(GUEST_VIRQ_OBJS) linker.ld Makefile
	$(CC) $(CFLAGS) $(GUEST_VIRQ_OBJS) $(LDFLAGS) -Wl,-Map=$(BUILD_DIR)/guest-virq.map -o $@
	$(OBJDUMP) -D $@ > $(BUILD_DIR)/guest-virq.list

$(BUILD_DIR)/guest-stage2-fault.elf: $(GUEST_STAGE2_FAULT_OBJS) linker.ld Makefile
	$(CC) $(CFLAGS) $(GUEST_STAGE2_FAULT_OBJS) $(LDFLAGS) -Wl,-Map=$(BUILD_DIR)/guest-stage2-fault.map -o $@
	$(OBJDUMP) -D $@ > $(BUILD_DIR)/guest-stage2-fault.list

define GUEST_SCHED_RULES
GUEST_SCHED$(1)_OBJS := $$(COMMON_OBJS) $$(BUILD_DIR)/sched$(1)/sched-main.o
DEPS += $$(GUEST_SCHED$(1)_OBJS:.o=.d)

guest-sched$(1).bin: $$(BUILD_DIR)/guest-sched$(1).elf
	$$(OBJCOPY) $$< -O binary $$@

$$(BUILD_DIR)/guest-sched$(1).elf: $$(GUEST_SCHED$(1)_OBJS) linker.ld Makefile
	$$(CC) $$(CFLAGS) $$(GUEST_SCHED$(1)_OBJS) $$(LDFLAGS) -Wl,--defsym=GUEST_BASE=$$(GUEST_SCHED_BASE_$(1)) -Wl,-Map=$$(BUILD_DIR)/guest-sched$(1).map -o $$@
	$$(OBJDUMP) -D $$@ > $$(BUILD_DIR)/guest-sched$(1).list

$$(BUILD_DIR)/sched$(1)/sched-main.o: sched-main.c
	@mkdir -p $$(@D)
	$$(CC) $$(CPPFLAGS) $$(CFLAGS) -DGUEST_ID=$(1) -c $$< -o $$@
endef

$(foreach id,$(GUEST_SCHED_IDS),$(eval $(call GUEST_SCHED_RULES,$(id))))

define GUEST_TIMER_RULES
GUEST_TIMER$(1)_OBJS := $$(COMMON_OBJS) $$(BUILD_DIR)/timer$(1)/timer-main.o
DEPS += $$(GUEST_TIMER$(1)_OBJS:.o=.d)

guest-timer$(1).bin: $$(BUILD_DIR)/guest-timer$(1).elf
	$$(OBJCOPY) $$< -O binary $$@

$$(BUILD_DIR)/guest-timer$(1).elf: $$(GUEST_TIMER$(1)_OBJS) linker.ld Makefile
	$$(CC) $$(CFLAGS) $$(GUEST_TIMER$(1)_OBJS) $$(LDFLAGS) -Wl,--defsym=GUEST_BASE=$$(GUEST_TIMER_BASE_$(1)) -Wl,-Map=$$(BUILD_DIR)/guest-timer$(1).map -o $$@
	$$(OBJDUMP) -D $$@ > $$(BUILD_DIR)/guest-timer$(1).list

$$(BUILD_DIR)/timer$(1)/timer-main.o: timer-main.c
	@mkdir -p $$(@D)
	$$(CC) $$(CPPFLAGS) $$(CFLAGS) -DGUEST_ID=$(1) -c $$< -o $$@
endef

$(foreach id,$(GUEST_TIMER_IDS),$(eval $(call GUEST_TIMER_RULES,$(id))))

define GUEST_MMIO_UART_RULES
GUEST_MMIO_UART$(1)_OBJS := $$(COMMON_OBJS) $$(BUILD_DIR)/mmio-uart$(1)/mmio-uart-main.o
DEPS += $$(GUEST_MMIO_UART$(1)_OBJS:.o=.d)

guest-mmio-uart$(1).bin: $$(BUILD_DIR)/guest-mmio-uart$(1).elf
	$$(OBJCOPY) $$< -O binary $$@

$$(BUILD_DIR)/guest-mmio-uart$(1).elf: $$(GUEST_MMIO_UART$(1)_OBJS) linker.ld Makefile
	$$(CC) $$(CFLAGS) $$(GUEST_MMIO_UART$(1)_OBJS) $$(LDFLAGS) -Wl,--defsym=GUEST_BASE=$$(GUEST_MMIO_UART_BASE_$(1)) -Wl,-Map=$$(BUILD_DIR)/guest-mmio-uart$(1).map -o $$@
	$$(OBJDUMP) -D $$@ > $$(BUILD_DIR)/guest-mmio-uart$(1).list

$$(BUILD_DIR)/mmio-uart$(1)/mmio-uart-main.o: mmio-uart-main.c
	@mkdir -p $$(@D)
	$$(CC) $$(CPPFLAGS) $$(CFLAGS) -DGUEST_ID=$(1) -c $$< -o $$@
endef

$(foreach id,$(GUEST_MMIO_UART_IDS),$(eval $(call GUEST_MMIO_UART_RULES,$(id))))

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/repo/src/lib/%.o: ../src/lib/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGETS:%=%.bin)

-include $(DEPS)
