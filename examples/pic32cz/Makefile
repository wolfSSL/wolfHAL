WHAL_DIR = $(PWD)/../../
INCLUDE = -I$(WHAL_DIR)
CFLAGS = -Wall -Werror $(INCLUDE) -g3 \
		 -ffreestanding -nostdlib -mcpu=cortex-m7 \
         -MMD -MF $(DEPDIR)/$*.d
LDFLAGS = --omagic -static

DEPDIR = .deps/

SOURCE ?= $(wildcard *.c)
SOURCE += $(wildcard $(WHAL_DIR)/src/*.c)

SOURCE += $(wildcard $(WHAL_DIR)/src/*/*.c)

OBJECTS = $(patsubst %.c,%.o,$(SOURCE))

DEPENDS = $(patsubst %.c,$(DEPDIR)/%.d,$(SOURCE))

GCC = $(GCC_PATH)arm-none-eabi-gcc
LD = $(GCC_PATH)arm-none-eabi-ld
OBJCOPY = $(GCC_PATH)arm-none-eabi-objcopy

LINKER_SCRIPT = linker.ld

all: boot.bin

%.d:
	@mkdir -p $(@D)

%.o: %.c Makefile
	$(GCC) $(CFLAGS) -c -o $@ $<

.SECONDARY:
%.elf: $(OBJECTS) $(LINKER_SCRIPT)
	$(LD) $(LDFLAGS) -T $(LINKER_SCRIPT) -o $@ $(OBJECTS)

%.bin: %.elf
	$(OBJCOPY) $^ -O binary $@

.PHONY: clean
clean:
	rm -rf $(DEPDIR) $(OBJECTS) boot.bin boot.elf

-include $(DEPENDS)
