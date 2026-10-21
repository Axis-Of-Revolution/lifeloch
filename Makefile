.SUFFIXES:
ifeq ($(strip $(DEVKITARM)),)
$(error Set DEVKITPRO and DEVKITARM to your devkitPro SDK paths)
endif
TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules
TARGET := lifeloch
BUILD := build
SOURCES := source
INCLUDES := include
# 0.10.10: (major << 10) | (minor << 4) | micro
CIA_VERSION ?= 170
APP_TITLE := Lifeloch
APP_DESCRIPTION := A Magic: The Gathering Game Tracker
APP_AUTHOR := Lifeloch
ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS := -g -Wall -Wextra -O2 -mword-relocations -ffunction-sections $(ARCH) $(INCLUDE) -D__3DS__
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++17
ASFLAGS := -g $(ARCH)
LDFLAGS = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS := -lcitro2d -lcitro3d -lctru -lm
LIBDIRS := $(CTRULIB)
ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(CURDIR)/source
export DEPSDIR := $(CURDIR)/$(BUILD)
CPPFILES := $(notdir $(wildcard source/*.cpp))
export LD := $(CXX)
export OFILES := $(CPPFILES:.cpp=.o)
export INCLUDE := -I$(CURDIR)/include $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export APP_ICON := $(TOPDIR)/assets/icon.png
export _3DSXFLAGS := --smdh=$(OUTPUT).smdh
.PHONY: all clean cia test
all:
	@mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(TOPDIR)/Makefile
cia: all assets/banner.bnr
	makerom -f cia -ver $(CIA_VERSION) -o $(TARGET).cia -rsf assets/application.rsf -target t -exefslogo -elf $(TARGET).elf -icon $(TARGET).smdh -banner assets/banner.bnr
assets/banner.bnr: assets/banner.png assets/silence.wav
	bannertool makebanner -i assets/banner.png -a assets/silence.wav -o $@
test:
	@mkdir -p build-host
	g++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Iinclude source/dice.cpp source/model.cpp source/storage.cpp source/ui.cpp source/pixel.cpp source/layout.cpp source/theme.cpp source/audio.cpp tests/test.cpp -o build-host/tests
	./build-host/tests
clean:
	rm -rf build $(TARGET).elf $(TARGET).3dsx $(TARGET).smdh $(TARGET).cia assets/banner.bnr
else
$(OUTPUT).3dsx: $(OUTPUT).elf $(OUTPUT).smdh
$(OUTPUT).elf: $(OFILES)
-include $(DEPSDIR)/*.d
endif
