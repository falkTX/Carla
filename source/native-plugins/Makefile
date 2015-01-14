#!/usr/bin/make -f
# Makefile for native-plugins #
# --------------------------- #
# Created by falkTX
#

CWD=..
MODULENAME=native-plugins
include ../modules/Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_C_FLAGS   += -I..
BUILD_CXX_FLAGS += -I.. -isystem $(CWD)/modules -I$(CWD)/modules/distrho

# ----------------------------------------------------------------------------------------------------------------------------
# Flags for _all.c

ALL_C_FLAGS  = $(BUILD_C_FLAGS)

# ZynAddSubFX
ifeq ($(HAVE_ZYN_DEPS),true)
ALL_C_FLAGS += -DWANT_ZYNADDSUBFX
endif

# Experimental plugins
ifeq ($(EXPERIMENTAL_PLUGINS),true)
ALL_C_FLAGS += -DWANT_EXPERIMENTAL_PLUGINS
endif

# ----------------------------------------------------------------------------------------------------------------------------
# Flags for ZynAddSubFX

ifeq ($(HAVE_ZYN_DEPS),true)
ZYN_CXX_FLAGS  = $(BUILD_CXX_FLAGS) -isystem zynaddsubfx
ZYN_CXX_FLAGS += $(shell pkg-config --cflags fftw3 mxml zlib)
ifneq ($(MACOS),true)
ZYN_CXX_FLAGS += -DHAVE_SCHEDULER
endif
ifeq ($(HAVE_ZYN_UI_DEPS),true)
ZYN_CXX_FLAGS += -DNTK_GUI -DWANT_ZYNADDSUBFX_UI
ZYN_CXX_FLAGS += $(shell pkg-config --cflags ntk_images ntk)
endif
endif

# ----------------------------------------------------------------------------------------------------------------------------
# Set objects

OBJS = \
	$(OBJDIR)/_all.c.o

# ----------------------------------------------------------------------------------------------------------------------------
# Simple plugins

OBJS += \
	$(OBJDIR)/bypass.c.o \
	$(OBJDIR)/lfo.c.o \
	$(OBJDIR)/midi-gain.c.o \
	$(OBJDIR)/midi-split.c.o \
	$(OBJDIR)/midi-through.c.o \
	$(OBJDIR)/midi-transpose.c.o \
	$(OBJDIR)/nekofilter.c.o

OBJS += \
	$(OBJDIR)/audio-file.cpp.o \
	$(OBJDIR)/bigmeter.cpp.o \
	$(OBJDIR)/midi-file.cpp.o \
	$(OBJDIR)/notes.cpp.o

# ----------------------------------------------------------------------------------------------------------------------------
# DISTRHO plugins

OBJS += \
	$(OBJDIR)/distrho-3bandeq.cpp.o \
	$(OBJDIR)/distrho-3bandsplitter.cpp.o \
	$(OBJDIR)/distrho-mverb.cpp.o \
	$(OBJDIR)/distrho-nekobi.cpp.o \
	$(OBJDIR)/distrho-pingpongpan.cpp.o

# ----------------------------------------------------------------------------------------------------------------------------
# ZynAddSubFX

ifeq ($(HAVE_ZYN_DEPS),true)
OBJS += \
	$(OBJDIR)/zynaddsubfx-fx.cpp.o \
	$(OBJDIR)/zynaddsubfx-synth.cpp.o \
	$(OBJDIR)/zynaddsubfx-src.cpp.o

ifeq ($(HAVE_ZYN_UI_DEPS),true)
OBJS += \
	$(OBJDIR)/zynaddsubfx-ui.cpp.o

ZYN_UI_FILES_CPP = \
	zynaddsubfx/UI/ADnoteUI.cpp \
	zynaddsubfx/UI/BankUI.cpp \
	zynaddsubfx/UI/ConfigUI.cpp \
	zynaddsubfx/UI/EffUI.cpp \
	zynaddsubfx/UI/EnvelopeUI.cpp \
	zynaddsubfx/UI/FilterUI.cpp \
	zynaddsubfx/UI/LFOUI.cpp \
	zynaddsubfx/UI/MasterUI.cpp \
	zynaddsubfx/UI/MicrotonalUI.cpp \
	zynaddsubfx/UI/OscilGenUI.cpp \
	zynaddsubfx/UI/PADnoteUI.cpp \
	zynaddsubfx/UI/PartUI.cpp \
	zynaddsubfx/UI/PresetsUI.cpp \
	zynaddsubfx/UI/ResonanceUI.cpp \
	zynaddsubfx/UI/SUBnoteUI.cpp \
	zynaddsubfx/UI/VirKeyboard.cpp

ZYN_UI_FILES_H = \
	zynaddsubfx/UI/ADnoteUI.h \
	zynaddsubfx/UI/BankUI.h \
	zynaddsubfx/UI/ConfigUI.h \
	zynaddsubfx/UI/EffUI.h \
	zynaddsubfx/UI/EnvelopeUI.h \
	zynaddsubfx/UI/FilterUI.h \
	zynaddsubfx/UI/LFOUI.h \
	zynaddsubfx/UI/MasterUI.h \
	zynaddsubfx/UI/MicrotonalUI.h \
	zynaddsubfx/UI/OscilGenUI.h \
	zynaddsubfx/UI/PADnoteUI.h \
	zynaddsubfx/UI/PartUI.h \
	zynaddsubfx/UI/PresetsUI.h \
	zynaddsubfx/UI/ResonanceUI.h \
	zynaddsubfx/UI/SUBnoteUI.h \
	zynaddsubfx/UI/VirKeyboard.h
endif
endif

# ----------------------------------------------------------------------------------------------------------------------------
# Experimental plugins

ifeq ($(EXPERIMENTAL_PLUGINS),true)
OBJS += \
	$(OBJDIR)/zita-jaaa.cpp.o
endif

# ----------------------------------------------------------------------------------------------------------------------------

all: $(MODULEDIR)/$(MODULENAME).a

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(MODULEDIR)/$(MODULENAME)*.a $(ZYN_UI_FILES_H) $(ZYN_UI_FILES_CPP)

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME).a: $(OBJS)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).a"
	@rm -f $@
	@$(AR) crs $@ $^

# ----------------------------------------------------------------------------------------------------------------------------

zynaddsubfx/UI/%.cpp: zynaddsubfx/UI/%.fl
	@echo "Generating $@/h"
	@ntk-fluid -c -o zynaddsubfx/UI/$*.cpp -h zynaddsubfx/UI/$*.h $<

zynaddsubfx/UI/%.h: zynaddsubfx/UI/%.fl
	@echo "Generating $@/cpp"
	@ntk-fluid -c -o zynaddsubfx/UI/$*.cpp -h zynaddsubfx/UI/$*.h $<

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/_all.c.o: _all.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CC) $< $(ALL_C_FLAGS) -c -o $@

$(OBJDIR)/%.c.o: %.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CC) $< $(BUILD_C_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/distrho-3bandeq.cpp.o: distrho-3bandeq.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DDISTRHO_NAMESPACE=DISTRHO_3BandEQ -Idistrho-3bandeq -I$(CWD)/modules/dgl -c -o $@

$(OBJDIR)/distrho-3bandsplitter.cpp.o: distrho-3bandsplitter.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DDISTRHO_NAMESPACE=DISTRHO_3BandSplitter -Idistrho-3bandsplitter -I$(CWD)/modules/dgl -c -o $@

$(OBJDIR)/distrho-mverb.cpp.o: distrho-mverb.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DDISTRHO_NAMESPACE=DISTRHO_MVerb -Idistrho-mverb -I$(CWD)/modules/dgl -c -o $@

$(OBJDIR)/distrho-nekobi.cpp.o: distrho-nekobi.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DDISTRHO_NAMESPACE=DISTRHO_Nekobi -Idistrho-nekobi -I$(CWD)/modules/dgl -c -o $@

$(OBJDIR)/distrho-pingpongpan.cpp.o: distrho-pingpongpan.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DDISTRHO_NAMESPACE=DISTRHO_PingPongPan -Idistrho-pingpongpan -I$(CWD)/modules/dgl -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/zynaddsubfx-fx.cpp.o: zynaddsubfx-fx.cpp $(ZYN_UI_FILES_H)
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(ZYN_CXX_FLAGS) -c -o $@

$(OBJDIR)/zynaddsubfx-synth.cpp.o: zynaddsubfx-synth.cpp $(ZYN_UI_FILES_H)
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(ZYN_CXX_FLAGS) -c -o $@

$(OBJDIR)/zynaddsubfx-src.cpp.o: zynaddsubfx-src.cpp $(ZYN_UI_FILES_H)
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(ZYN_CXX_FLAGS) -w -c -o $@

$(OBJDIR)/zynaddsubfx-ui.cpp.o: zynaddsubfx-ui.cpp $(ZYN_UI_FILES_H) $(ZYN_UI_FILES_CPP)
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(ZYN_CXX_FLAGS) -w -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

-include $(OBJS:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
