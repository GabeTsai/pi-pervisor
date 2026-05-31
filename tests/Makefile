ARMGNU ?= arm-none-eabi
ARMCPU ?= cortex-a53

ROOT := ..

CC      := $(ARMGNU)-gcc
LD      := $(ARMGNU)-gcc
OBJCOPY := $(ARMGNU)-objcopy
OBJDUMP := $(ARMGNU)-objdump

# Build tests/foo.c with: make -C tests PROG=foo
PROG ?= test-boot-handoff

BUILD_DIR := build/$(PROG)
ELF       := $(BUILD_DIR)/$(PROG).elf
MAP       := $(BUILD_DIR)/$(PROG).map
LIST      := $(BUILD_DIR)/$(PROG).list
BIN       := $(PROG).bin

INC_DIRS := \
	$(ROOT)/include \
	$(ROOT)/include/arch \
	$(ROOT)/include/platform \
	$(ROOT)/include/drivers \
	$(ROOT)/include/lib

TEST_SRC := $(PROG).c
REPO_C_SRCS := \
	$(ROOT)/src/arch/mmio.c \
	$(ROOT)/src/drivers/gpio.c \
	$(ROOT)/src/drivers/timer.c \
	$(ROOT)/src/drivers/uart.c \
	$(ROOT)/src/lib/check.c \
	$(ROOT)/src/lib/printk.c \
	$(ROOT)/src/platform/irq.c \
	$(ROOT)/src/platform/platform.c
REPO_S_SRCS := $(ROOT)/src/boot/start.S

TEST_OBJ := $(BUILD_DIR)/test/$(PROG).o
REPO_C_OBJS := $(patsubst $(ROOT)/%.c,$(BUILD_DIR)/repo/%.o,$(REPO_C_SRCS))
REPO_S_OBJS := $(patsubst $(ROOT)/%.S,$(BUILD_DIR)/repo/%.o,$(REPO_S_SRCS))
OBJS := $(TEST_OBJ) $(REPO_C_OBJS) $(REPO_S_OBJS)
DEPS := $(OBJS:.o=.d)

CPPFLAGS := $(addprefix -I,$(INC_DIRS)) -MMD -MP
CFLAGS   := -O2 -Wall -Wextra -Wno-unused-parameter -Werror -std=gnu99
CFLAGS   += -nostdlib -nostartfiles -ffreestanding
CFLAGS   += -mcpu=$(ARMCPU) -marm -mno-unaligned-access
ASFLAGS  := $(CFLAGS)
LDFLAGS  := -T $(ROOT)/linker.ld -nostdlib -nostartfiles -ffreestanding
LDFLAGS  += -Wl,-Map=$(MAP)

.PHONY: all clean

all: $(BIN)

$(BIN): $(ELF)
	$(OBJCOPY) $< -O binary $@

$(ELF): $(OBJS) $(ROOT)/linker.ld
	@mkdir -p $(@D)
	$(LD) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@
	$(OBJDUMP) -D $@ > $(LIST)

$(TEST_OBJ): $(TEST_SRC)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/repo/%.o: $(ROOT)/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/repo/%.o: $(ROOT)/%.S
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@

clean:
	rm -rf build *.bin

-include $(DEPS)
