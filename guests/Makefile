ARMGNU ?= arm-none-eabi
ARMCPU ?= cortex-a53

CC := $(ARMGNU)-gcc
OBJCOPY := $(ARMGNU)-objcopy
OBJDUMP := $(ARMGNU)-objdump

BUILD_DIR := build
TARGETS := guest guest-virq

COMMON_S_SRCS := start.S guest-vectors.S lib/guest_hypercall.S
COMMON_OBJS := $(patsubst %.S,$(BUILD_DIR)/%.o,$(COMMON_S_SRCS))
COMMON_OBJS += $(BUILD_DIR)/lib/printk.o $(BUILD_DIR)/repo/src/lib/printk_format.o
GUEST_OBJS := $(COMMON_OBJS) $(BUILD_DIR)/main.o
GUEST_VIRQ_OBJS := $(COMMON_OBJS) $(BUILD_DIR)/virq-main.o
DEPS := $(GUEST_OBJS:.o=.d) $(GUEST_VIRQ_OBJS:.o=.d)

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

$(BUILD_DIR)/guest.elf: $(GUEST_OBJS) linker.ld
	$(CC) $(CFLAGS) $(GUEST_OBJS) $(LDFLAGS) -Wl,-Map=$(BUILD_DIR)/guest.map -o $@
	$(OBJDUMP) -D $@ > $(BUILD_DIR)/guest.list

$(BUILD_DIR)/guest-virq.elf: $(GUEST_VIRQ_OBJS) linker.ld
	$(CC) $(CFLAGS) $(GUEST_VIRQ_OBJS) $(LDFLAGS) -Wl,-Map=$(BUILD_DIR)/guest-virq.map -o $@
	$(OBJDUMP) -D $@ > $(BUILD_DIR)/guest-virq.list

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
