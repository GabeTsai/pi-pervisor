ARMGNU ?= arm-none-eabi
ARMCPU ?= cortex-a53

ROOT := ..

CC      := $(ARMGNU)-gcc
LD      := $(ARMGNU)-gcc
OBJCOPY := $(ARMGNU)-objcopy
OBJDUMP := $(ARMGNU)-objdump

# Build one test with: make -C tests PROG=foo
# Build all tests with: make -C tests
PROG_FROM_CLI := $(filter command line,$(origin PROG))
PROG ?= test-basic-hvc

TESTS := \
	test-basic-hvc \
	test-hyp-exceptions \
	test-hyp-undef \
	test-hyp-prefetch-abort \
	test-hyp-data-abort \
	test-hyp-enter-lower \
	test-hyp-irq \
	test-hyp-generic-timer \
	test-basic-guest-hvc \
	test-guest-image \
	test-guest-virq-image \
	test-guest-sched-virq-image \
	test-guest-sched-yield \
	test-guest-sched-timer \
	test-guest-stage2-fault \

ifeq ($(PROG_FROM_CLI),command line)
ACTIVE_TESTS := $(PROG)
else
ACTIVE_TESTS := $(TESTS)
endif

REPO_BUILD_DIR := build/repo

INC_DIRS := \
	$(ROOT)/include \
	$(ROOT)/include/arch \
	$(ROOT)/include/platform \
	$(ROOT)/include/drivers \
	$(ROOT)/include/lib \
	$(ROOT)/include/hv \
	$(ROOT)/guests/lib

REPO_C_SRCS := \
	$(ROOT)/src/arch/mmio.c \
	$(ROOT)/src/arch/hyp-regs.c \
	$(ROOT)/src/arch/hyp-exceptions.c \
	$(ROOT)/src/drivers/gpio.c \
	$(ROOT)/src/drivers/generic_timer.c \
	$(ROOT)/src/drivers/timer.c \
	$(ROOT)/src/drivers/uart.c \
	$(ROOT)/src/lib/check.c \
	$(ROOT)/src/lib/printk.c \
	$(ROOT)/src/lib/printk_format.c \
	$(ROOT)/src/platform/irq.c \
	$(ROOT)/src/platform/platform.c \
	$(ROOT)/src/hv/hypercall.c \
	$(ROOT)/src/hv/scheduler.c \
	$(ROOT)/src/hv/mmu.c \
	$(ROOT)/src/hv/stage2.c \
	$(ROOT)/src/hv/vcpu.c \
	$(ROOT)/src/hv/virq.c \
	$(ROOT)/src/hv/vm.c
REPO_S_SRCS := \
	$(ROOT)/src/boot/start.S \
	$(ROOT)/src/arch/hyp-vectors.S \
	$(ROOT)/src/arch/hyp-banked-regs.S \
	$(ROOT)/src/arch/hyp-enter-vcpu.S \
	$(ROOT)/src/arch/hyp-enter-lower.S \
	$(ROOT)/src/hv/idle-vcpu.S \
	$(ROOT)/guests/lib/guest_hypercall.S

REPO_C_OBJS := $(patsubst $(ROOT)/%.c,$(REPO_BUILD_DIR)/%.o,$(REPO_C_SRCS))
REPO_S_OBJS := $(patsubst $(ROOT)/%.S,$(REPO_BUILD_DIR)/%.o,$(REPO_S_SRCS))
REPO_OBJS := $(REPO_C_OBJS) $(REPO_S_OBJS)
DEPS := $(REPO_OBJS:.o=.d)

CPPFLAGS := $(addprefix -I,$(INC_DIRS)) -MMD -MP
CFLAGS   := -O2 -Wall -Wextra -Wno-unused-parameter -Werror -std=gnu99
CFLAGS   += -nostdlib -nostartfiles -ffreestanding
CFLAGS   += -mcpu=$(ARMCPU) -marm -mno-unaligned-access
ASFLAGS  := $(CFLAGS)
LDFLAGS  := -T $(ROOT)/linker.ld -nostdlib -nostartfiles -ffreestanding

.PHONY: all all-tests clean $(ACTIVE_TESTS)

ifeq ($(PROG_FROM_CLI),command line)
all: $(PROG).bin
else
all: all-tests
endif

all-tests: $(TESTS)

define TEST_RULES
$(1)_BUILD_DIR := build/tests/$(1)
$(1)_ELF       := $$($(1)_BUILD_DIR)/$(1).elf
$(1)_MAP       := $$($(1)_BUILD_DIR)/$(1).map
$(1)_LIST      := $$($(1)_BUILD_DIR)/$(1).list
$(1)_TEST_OBJ  := $$($(1)_BUILD_DIR)/test/$(1).o
$(1)_OBJS      := $$($(1)_TEST_OBJ) $$(TEST_EXTRA_OBJS_$(1)) $$(REPO_OBJS)

$(1): $(1).bin

$(1).bin: $$($(1)_ELF)
	$$(OBJCOPY) $$< -O binary $$@

$$($(1)_ELF): $$($(1)_OBJS) $$(ROOT)/linker.ld
	@mkdir -p $$(@D)
	$$(LD) $$(CFLAGS) $$($(1)_OBJS) $$(LDFLAGS) -Wl,-Map=$$($(1)_MAP) -o $$@
	$$(OBJDUMP) -D $$@ > $$($(1)_LIST)

$$($(1)_TEST_OBJ): $(1).c
	@mkdir -p $$(@D)
	$$(CC) $$(CPPFLAGS) $$(CFLAGS) -c $$< -o $$@

DEPS += $$($(1)_TEST_OBJ:.o=.d)
endef

TEST_EXTRA_OBJS_test-hyp-enter-lower := \
	build/tests/test-hyp-enter-lower/test/test-hyp-enter-lower.S.o

TEST_EXTRA_OBJS_test-hyp-irq := \
	build/tests/test-basic-guest-hvc/test/test-basic-guest-hvc.S.o

TEST_EXTRA_OBJS_test-hyp-generic-timer := \
	build/tests/test-basic-guest-hvc/test/test-basic-guest-hvc.S.o

TEST_EXTRA_OBJS_test-basic-guest-hvc := \
	build/tests/test-basic-guest-hvc/test/test-basic-guest-hvc.S.o \
	build/tests/test-basic-guest-hvc/guest/main.o

TEST_EXTRA_OBJS_test-guest-image := \
	build/tests/test-guest-image/test/test-guest-image.S.o

TEST_EXTRA_OBJS_test-guest-virq-image := \
	build/tests/test-guest-virq-image/test/test-guest-virq-image.S.o

TEST_EXTRA_OBJS_test-guest-sched-virq-image := \
	build/tests/test-guest-virq-image/test/test-guest-virq-image.S.o

TEST_EXTRA_OBJS_test-guest-sched-yield := \
	build/tests/test-guest-sched-yield/test/test-guest-sched-yield.S.o

TEST_EXTRA_OBJS_test-guest-sched-timer := \
	build/tests/test-guest-sched-timer/test/test-guest-sched-timer.S.o

TEST_EXTRA_OBJS_test-guest-stage2-fault := \
	build/tests/test-guest-stage2-fault/test/test-guest-stage2-fault.S.o

$(foreach test,$(ACTIVE_TESTS),$(eval $(call TEST_RULES,$(test))))

build/tests/test-hyp-enter-lower/test/test-hyp-enter-lower.S.o: test-hyp-enter-lower.S
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@

build/tests/test-basic-guest-hvc/test/test-basic-guest-hvc.S.o: test-basic-guest-hvc.S
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@

build/tests/test-basic-guest-hvc/guest/main.o: $(ROOT)/guests/main.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/tests/test-guest-image/test/test-guest-image.S.o: test-guest-image.S $(ROOT)/guests/guest.bin
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@

build/tests/test-guest-virq-image/test/test-guest-virq-image.S.o: test-guest-virq-image.S $(ROOT)/guests/guest-virq.bin
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@

build/tests/test-guest-sched-yield/test/test-guest-sched-yield.S.o: test-guest-sched-yield.S $(ROOT)/guests/guest-sched0.bin $(ROOT)/guests/guest-sched1.bin
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@

build/tests/test-guest-sched-timer/test/test-guest-sched-timer.S.o: test-guest-sched-timer.S $(ROOT)/guests/guest-timer0.bin $(ROOT)/guests/guest-timer1.bin
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@

build/tests/test-guest-stage2-fault/test/test-guest-stage2-fault.S.o: test-guest-stage2-fault.S $(ROOT)/guests/guest-stage2-fault.bin
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@

$(ROOT)/guests/guest.bin: FORCE
	$(MAKE) -C $(ROOT)/guests guest.bin ARMGNU=$(ARMGNU) ARMCPU=$(ARMCPU)

$(ROOT)/guests/guest-virq.bin: FORCE
	$(MAKE) -C $(ROOT)/guests guest-virq.bin ARMGNU=$(ARMGNU) ARMCPU=$(ARMCPU)

$(ROOT)/guests/guest-sched0.bin: FORCE
	$(MAKE) -C $(ROOT)/guests guest-sched0.bin ARMGNU=$(ARMGNU) ARMCPU=$(ARMCPU)

$(ROOT)/guests/guest-sched1.bin: FORCE
	$(MAKE) -C $(ROOT)/guests guest-sched1.bin ARMGNU=$(ARMGNU) ARMCPU=$(ARMCPU)

$(ROOT)/guests/guest-timer0.bin: FORCE
	$(MAKE) -C $(ROOT)/guests guest-timer0.bin ARMGNU=$(ARMGNU) ARMCPU=$(ARMCPU)

$(ROOT)/guests/guest-timer1.bin: FORCE
	$(MAKE) -C $(ROOT)/guests guest-timer1.bin ARMGNU=$(ARMGNU) ARMCPU=$(ARMCPU)

$(ROOT)/guests/guest-stage2-fault.bin: FORCE
	$(MAKE) -C $(ROOT)/guests guest-stage2-fault.bin ARMGNU=$(ARMGNU) ARMCPU=$(ARMCPU)

$(REPO_BUILD_DIR)/%.o: $(ROOT)/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(REPO_BUILD_DIR)/%.o: $(ROOT)/%.S
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@

clean:
	rm -rf build *.bin

-include $(DEPS)

.PHONY: FORCE
FORCE:
