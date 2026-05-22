ARMGNU ?= arm-none-eabi
ARMCPU ?= cortex-a53

ROOT := ..

ARM_CC      := $(ARMGNU)-gcc
ARM_LD      := $(ARMGNU)-gcc
ARM_OBJCOPY := $(ARMGNU)-objcopy
ARM_OBJDUMP := $(ARMGNU)-objdump
HOST_CC     ?= cc

BUILD_DIR      := build
PI_BUILD_DIR   := $(BUILD_DIR)/pi
HOST_BUILD_DIR := $(BUILD_DIR)/host

PI_TARGET := bootloader
PI_ELF    := $(PI_BUILD_DIR)/$(PI_TARGET).elf
PI_BIN    := $(BUILD_DIR)/bootloader.bin
PI_IMAGE  := kernel.img

HOST_TARGET := rpi-install
HOST_BIN    := $(HOST_TARGET)

PREFIX ?= $(HOME)/.local
BINDIR ?= $(PREFIX)/bin

PI_INC_DIRS := include \
	$(ROOT)/include \
	$(ROOT)/include/arch \
	$(ROOT)/include/platform \
	$(ROOT)/include/drivers \
	$(ROOT)/include/lib

PI_LOCAL_C_SRCS := src/main.c
PI_LOCAL_S_SRCS := src/boot-start.S
PI_REPO_C_SRCS := \
	$(ROOT)/src/arch/mmio.c \
	$(ROOT)/src/drivers/gpio.c \
	$(ROOT)/src/drivers/timer.c \
	$(ROOT)/src/drivers/uart.c \
	$(ROOT)/src/lib/check.c \
	$(ROOT)/src/platform/irq.c \
	$(ROOT)/src/platform/platform.c

PI_LOCAL_C_OBJS := $(patsubst %.c,$(PI_BUILD_DIR)/local/%.o,$(PI_LOCAL_C_SRCS))
PI_LOCAL_S_OBJS := $(patsubst %.S,$(PI_BUILD_DIR)/local/%.o,$(PI_LOCAL_S_SRCS))
PI_REPO_C_OBJS  := $(patsubst $(ROOT)/%.c,$(PI_BUILD_DIR)/repo/%.o,$(PI_REPO_C_SRCS))
PI_OBJS         := $(PI_LOCAL_C_OBJS) $(PI_LOCAL_S_OBJS) $(PI_REPO_C_OBJS)

PI_CPPFLAGS := $(addprefix -I,$(PI_INC_DIRS)) -MMD -MP
PI_CFLAGS   := -O2 -Wall -Wextra -Wno-unused-parameter -Werror -std=gnu99
PI_CFLAGS   += -nostdlib -nostartfiles -ffreestanding
PI_CFLAGS   += -mcpu=$(ARMCPU) -marm -mno-unaligned-access
PI_ASFLAGS  := $(PI_CFLAGS)
PI_LDFLAGS  := -T linker.ld -nostdlib -nostartfiles -ffreestanding
PI_LDFLAGS  += -Wl,-Map=$(PI_BUILD_DIR)/$(PI_TARGET).map

HOST_INC_DIRS := include
HOST_SRCS := \
	src/rpi-install.c \
	src/put-code.c \
	src/boot-unix-io.c \
	src/tty-helpers.c \
	src/pi-echo.c
HOST_OBJS := $(patsubst %.c,$(HOST_BUILD_DIR)/%.o,$(HOST_SRCS))

HOST_CPPFLAGS := $(addprefix -I,$(HOST_INC_DIRS)) -MMD -MP
HOST_CFLAGS   ?= -O2 -Wall -Wextra -std=gnu99

DEPS := $(PI_OBJS:.o=.d) $(HOST_OBJS:.o=.d)

.PHONY: all image host install-host clean

all: image host

image: $(PI_IMAGE) $(PI_BIN)

host: $(HOST_BIN)

$(PI_IMAGE): $(PI_BIN)
	cp $< $@

$(PI_BIN): $(PI_ELF)
	$(ARM_OBJCOPY) $< -O binary $@

$(PI_ELF): $(PI_OBJS) linker.ld
	@mkdir -p $(@D)
	$(ARM_LD) $(PI_CFLAGS) $(PI_OBJS) $(PI_LDFLAGS) -o $@
	$(ARM_OBJDUMP) -D $@ > $(PI_BUILD_DIR)/$(PI_TARGET).list

$(PI_BUILD_DIR)/local/%.o: %.c
	@mkdir -p $(@D)
	$(ARM_CC) $(PI_CPPFLAGS) $(PI_CFLAGS) -c $< -o $@

$(PI_BUILD_DIR)/local/%.o: %.S
	@mkdir -p $(@D)
	$(ARM_CC) $(PI_CPPFLAGS) $(PI_ASFLAGS) -c $< -o $@

$(PI_BUILD_DIR)/repo/%.o: $(ROOT)/%.c
	@mkdir -p $(@D)
	$(ARM_CC) $(PI_CPPFLAGS) $(PI_CFLAGS) -c $< -o $@

$(HOST_BIN): $(HOST_OBJS)
	$(HOST_CC) $(HOST_OBJS) -o $@

$(HOST_BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(HOST_CC) $(HOST_CPPFLAGS) $(HOST_CFLAGS) -c $< -o $@

# puts rpi-install binary in user's bin directory
install-host: $(HOST_BIN)
	@mkdir -p $(DESTDIR)$(BINDIR)
	cp $(HOST_BIN) $(DESTDIR)$(BINDIR)/$(HOST_BIN)

clean:
	rm -rf $(BUILD_DIR) $(PI_IMAGE) $(HOST_BIN)

-include $(DEPS)
