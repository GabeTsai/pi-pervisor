ARMGNU ?= arm-none-eabi
ARMCPU ?= cortex-a53

CC      := $(ARMGNU)-gcc
LD      := $(ARMGNU)-gcc
OBJCOPY := $(ARMGNU)-objcopy
OBJDUMP := $(ARMGNU)-objdump
NM      := $(ARMGNU)-nm

BUILD_DIR := build
TARGET    := kernel
IMAGE     := kernel.img
FIRMWARE_DIR ?= firmware

SRC_DIRS := src
INC_DIRS := include include/arch include/platform include/drivers include/lib

C_SRCS := $(shell find $(SRC_DIRS) -name '*.c')
S_SRCS := $(shell find $(SRC_DIRS) -name '*.S')
OBJS   := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRCS))
OBJS   += $(patsubst %.S,$(BUILD_DIR)/%.o,$(S_SRCS))
DEPS   := $(OBJS:.o=.d)

CPPFLAGS := $(addprefix -I,$(INC_DIRS)) -MMD -MP
CFLAGS   := -O2 -Wall -Wextra -Wno-unused-parameter -Werror -std=gnu99
CFLAGS   += -nostdlib -nostartfiles -ffreestanding
CFLAGS   += -mcpu=$(ARMCPU) -marm -mno-unaligned-access
ASFLAGS  := $(CFLAGS)
LDFLAGS  := -T linker.ld -nostdlib -nostartfiles -ffreestanding
LDFLAGS  += -Wl,-Map=$(BUILD_DIR)/$(TARGET).map

.PHONY: all clean test firmware

all: $(IMAGE)

$(IMAGE): $(BUILD_DIR)/$(TARGET).img
	cp $< $@

$(BUILD_DIR)/$(TARGET).img: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) $< -O binary $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) linker.ld
	@mkdir -p $(@D)
	$(LD) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@
	$(OBJDUMP) -D $@ > $(BUILD_DIR)/$(TARGET).list

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@

test: all
	test -s $(IMAGE)
	$(NM) $(BUILD_DIR)/$(TARGET).elf | grep -q ' T main$$'

firmware: test
	cp $(IMAGE) $(FIRMWARE_DIR)/$(IMAGE)
	shasum -a 256 $(IMAGE) $(FIRMWARE_DIR)/$(IMAGE)

clean:
	rm -rf $(BUILD_DIR) $(IMAGE)

-include $(DEPS)
