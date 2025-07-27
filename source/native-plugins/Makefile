#!/usr/bin/make -f
# Makefile for native-plugins #
# --------------------------- #
# Created by falkTX
#

CWD=..
CWDE=..
MODULENAME=native-plugins
include ../modules/Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_C_FLAGS   += -I..
BUILD_CXX_FLAGS += -I.. -I$(CWD)/modules

ifneq ($(HAIKU),true)
ifneq ($(WASM),true)
BUILD_CXX_FLAGS += -pthread
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set targets

TARGETS = $(MODULEDIR)/$(MODULENAME).a

BUNDLES = \
	$(MODULEDIR)/$(MODULENAME)_audiogain.a \
	$(MODULEDIR)/$(MODULENAME)_files.a \
	$(MODULEDIR)/$(MODULENAME)_miditools.a

# ---------------------------------------------------------------------------------------------------------------------
# Set objects

OBJS = \
	$(OBJDIR)/_all.c.o \
	$(OBJDIR)/_data.cpp.o \
	$(OBJDIR)/audio-gain.c.o \
	$(OBJDIR)/bypass.c.o \
	$(OBJDIR)/cv-to-audio.c.o \
	$(OBJDIR)/lfo.c.o \
	$(OBJDIR)/quadpanner.c.o \
	$(OBJDIR)/midi-channel-filter.c.o \
	$(OBJDIR)/midi-channel-ab.c.o \
	$(OBJDIR)/midi-channelize.c.o \
	$(OBJDIR)/midi-gain.c.o \
	$(OBJDIR)/midi-join.c.o \
	$(OBJDIR)/midi-split.c.o \
	$(OBJDIR)/midi-to-cv.c.o \
	$(OBJDIR)/midi-through.c.o \
	$(OBJDIR)/midi-transpose.c.o \
	$(OBJDIR)/audio-file.cpp.o \
	$(OBJDIR)/bigmeter.cpp.o \
	$(OBJDIR)/midi-file.cpp.o \
	$(OBJDIR)/midi-pattern.cpp.o \
	$(OBJDIR)/notes.cpp.o \
	$(OBJDIR)/xycontroller.cpp.o

OBJS_audiogain = \
	$(OBJDIR)/audio-gain.c.o

OBJS_files = \
	$(OBJDIR)/audio-file.cpp.o \
	$(OBJDIR)/midi-file.cpp.o

OBJS_miditools = \
	$(OBJDIR)/midi-channel-filter.c.o \
	$(OBJDIR)/midi-channel-ab.c.o \
	$(OBJDIR)/midi-channelize.c.o \
	$(OBJDIR)/midi-gain.c.o \
	$(OBJDIR)/midi-join.c.o \
	$(OBJDIR)/midi-split.c.o \
	$(OBJDIR)/midi-transpose.c.o

# ---------------------------------------------------------------------------------------------------------------------
# Include external plugins, if present

ifeq ($(EXTERNAL_PLUGINS),true)
ifneq (,$(wildcard external/Makefile.mk2))
include external/Makefile.mk2
else
$(error External plugins build requested but submodule not present, cannot build)
endif
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)
bundles: $(BUNDLES)

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(MODULEDIR)/$(MODULENAME)*.a $(ZYN_UI_FILES_H) $(ZYN_UI_FILES_CPP) $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME).a: $(OBJS)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME)_audiogain.a: $(OBJS_audiogain)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME)_audiogain.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME)_files.a: $(OBJS_files)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME)_files.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME)_miditools.a: $(OBJS_miditools)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME)_miditools.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.c.o: %.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CC) $< $(BUILD_C_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJS:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
