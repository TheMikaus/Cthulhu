.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "DEVKITARM must point to devkitARM")
endif
ifeq ($(strip $(CTRPF_ROOT)),)
$(error "CTRPF_ROOT must point to the pinned CTRPluginFramework template")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET := CthulhuHomeMenu
PLGINFO := CTRPluginFramework.plgInfo
BUILD := build
SOURCES := source
INCLUDES := include
ARCH := -march=armv6k -mlittle-endian -mtune=mpcore -mfloat-abi=hard
CFLAGS := -Os -mword-relocations -fomit-frame-pointer -ffunction-sections \
          -fno-strict-aliasing $(ARCH) $(INCLUDE) -D__3DS__
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11
ASFLAGS := $(ARCH)
LDFLAGS := -T $(TOPDIR)/3ds.ld $(ARCH) -Os -Wl,-Map,$(TARGET).map,--gc-sections,--strip-discarded,--strip-debug
LIBS := -lctrpf -lctru

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(CURDIR)/$(SOURCES)
export DEPSDIR := $(CURDIR)/$(BUILD)
CPPFILES := $(notdir $(wildcard $(SOURCES)/*.cpp))
SFILES := $(filter-out bootloader.s,$(notdir $(wildcard $(SOURCES)/*.s)))
export LD := $(CXX)
export OFILES := $(CPPFILES:.cpp=.o) $(SFILES:.s=.o)
export INCLUDE := -I $(CURDIR)/$(INCLUDES) -I $(CTRPF_ROOT)/include \
                  -I $(DEVKITPRO)/libctru/include -I $(CURDIR)/$(BUILD)
export LIBPATHS := -L $(CTRPF_ROOT)/lib -L $(DEVKITPRO)/libctru/lib

.PHONY: all clean $(BUILD)
all: $(BUILD)
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $@ -f $(CURDIR)/Makefile
clean:
	@rm -rf $(BUILD) $(TARGET).3gx $(TARGET).elf $(TARGET).map objdump
else
DEPENDS := $(OFILES:.o=.d)
$(OUTPUT).3gx: $(OFILES)
%.3gx: %.elf
	@echo creating $(notdir $@)
	@3gxtool -s $< $(TOPDIR)/$(PLGINFO) $@
-include $(DEPENDS)
endif
