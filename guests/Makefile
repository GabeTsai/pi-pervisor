ARMGNU ?= arm-none-eabi
ARMCPU ?= cortex-a53

CC := $(ARMGNU)-gcc
OBJCOPY := $(ARMGNU)-objcopy
OBJDUMP := $(ARMGNU)-objdump

BUILD_DIR := build
TARGET := guest

SRCS := start.S main.c lib/guest_hypercall.S
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(filter %.c,$(SRCS)))
OBJS += $(patsubst %.S,$(BUILD_DIR)/%.o,$(filter %.S,$(SRCS)))
DEPS := $(OBJS:.o=.d)

CPPFLAGS := -I../include -I../include/arch -I../include/hv -Ilib -MMD -MP
CFLAGS := -O2 -Wall -Wextra -Werror -ffreestanding -nostdlib -nostartfiles
CFLAGS += -std=gnu99 -mcpu=$(ARMCPU) -marm -mno-unaligned-access
LDFLAGS := -T linker.ld -nostdlib -nostartfiles -ffreestanding

.PHONY: all clean

all: $(TARGET).bin

$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) $< -O binary $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) linker.ld
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map -o $@
	$(OBJDUMP) -D $@ > $(BUILD_DIR)/$(TARGET).list

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET).bin

-include $(DEPS)
