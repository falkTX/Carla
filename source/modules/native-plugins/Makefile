#!/usr/bin/make -f
# Makefile for native-plugins #
# --------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_C_FLAGS   += -I. -I../../includes -isystem ..
BUILD_CXX_FLAGS += -I. -I../../includes -I../../utils -I../distrho -isystem ..

# --------------------------------------------------------------
# Flags for _all.c

ALL_C_FLAGS  = $(BUILD_C_FLAGS)

# AudioFile
ifeq ($(HAVE_AF_DEPS),true)
ALL_C_FLAGS += -DWANT_AUDIOFILE
endif

# MidiFile
ifeq ($(HAVE_MF_DEPS),true)
ALL_C_FLAGS += -DWANT_MIDIFILE
endif

# ProjectM
ifeq ($(HAVE_PM_DEPS),true)
ALL_C_FLAGS += -DWANT_PROJECTM
endif

# ZynAddSubFX
ifeq ($(HAVE_ZYN_DEPS),true)
ALL_C_FLAGS += -DWANT_ZYNADDSUBFX
endif

# --------------------------------------------------------------
# Flags for MidiFile

ifeq ($(HAVE_MF_DEPS),true)
MF_CXX_FLAGS  = $(BUILD_CXX_FLAGS)
MF_CXX_FLAGS += $(shell pkg-config --cflags smf)
endif

# --------------------------------------------------------------
# Flags for ProjectM

ifeq ($(HAVE_PM_DEPS),true)
PM_CXX_FLAGS  = $(BUILD_CXX_FLAGS)
PM_CXX_FLAGS += $(shell pkg-config --cflags libprojectM)
endif

# --------------------------------------------------------------
# Flags for ZynAddSubFX

ifeq ($(HAVE_ZYN_DEPS),true)
ZYN_CXX_FLAGS  = $(BUILD_CXX_FLAGS)
ZYN_CXX_FLAGS += $(shell pkg-config --cflags fftw3 mxml zlib)
ifneq ($(MACOS),true)
ZYN_CXX_FLAGS += -DHAVE_SCHEDULER
endif
ifeq ($(HAVE_ZYN_UI_DEPS),true)
ZYN_CXX_FLAGS += -DNTK_GUI -DWANT_ZYNADDSUBFX_UI
ZYN_CXX_FLAGS += $(shell pkg-config --cflags ntk_images ntk)
endif
endif

# --------------------------------------------------------------
# Set objects

OBJS = _all.c.o

# --------------------------------------------------------------
# Simple plugins

OBJS += \
	bypass.c.o \
	lfo.c.o \
	midi-gain.c.o \
	midi-split.c.o \
	midi-through.c.o \
	midi-transpose.c.o \
	nekofilter.c.o

ifneq ($(WIN32),true)
# --------------------------------------------------------------
# External-UI plugins

OBJS += \
	bigmeter.cpp.o \
	notes.cpp.o
endif

ifeq ($(HAVE_JUCE_UI),true)
# --------------------------------------------------------------
# JUCE based plugins

OBJS += \
	juce-patchbay.cpp.o \
	vex-fx.cpp.o \
	vex-synth.cpp.o \
	vex-src.cpp.o
endif

# --------------------------------------------------------------
# AudioFile

ifeq ($(HAVE_AF_DEPS),true)
OBJS += audio-file.cpp.o
endif

# --------------------------------------------------------------
# MidiFile

ifeq ($(HAVE_MF_DEPS),true)
OBJS += midi-file.cpp.o
endif

# --------------------------------------------------------------
# ZynAddSubFX

ifeq ($(HAVE_ZYN_DEPS),true)
OBJS += \
	zynaddsubfx-fx.cpp.o \
	zynaddsubfx-synth.cpp.o \
	zynaddsubfx-src.cpp.o

ifeq ($(HAVE_ZYN_UI_DEPS),true)
OBJS += zynaddsubfx-ui.cpp.o

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

# --------------------------------------------------------------

all: ../native-plugins.a

# --------------------------------------------------------------

../native-plugins.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

# --------------------------------------------------------------

CDEPS   = ../CarlaNative.h

_all.c.o: _all.c $(CDEPS)
	$(CC) $< $(ALL_C_FLAGS) -c -o $@

bypass.c.o: bypass.c $(CDEPS)
	$(CC) $< $(BUILD_C_FLAGS) -c -o $@

lfo.c.o: lfo.c $(CDEPS)
	$(CC) $< $(BUILD_C_FLAGS) -c -o $@

midi-%.c.o: midi-%.c $(CDEPS)
	$(CC) $< $(BUILD_C_FLAGS) -c -o $@

nekofilter.c.o: nekofilter.c nekofilter/*.c nekofilter/*.h $(CDEPS)
	$(CC) $< $(BUILD_C_FLAGS) -c -o $@

# --------------------------------------------------------------

CXXDEPS = ../CarlaNative.h ../CarlaNative.hpp

audio-file.cpp.o: audio-file.cpp audio-base.hpp $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

bigmeter.cpp.o: bigmeter.cpp $(CXXDEPS) ../CarlaNativeExtUI.hpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

distrho-3bandeq.cpp.o: distrho-3bandeq.cpp 3bandeq/*.cpp 3bandeq/*.h 3bandeq/*.hpp ../distrho/* $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -I../dgl -I3bandeq -DDISTRHO_NAMESPACE=DISTRHO_3BandEQ -c -o $@

distrho-3bandsplitter.cpp.o: distrho-3bandsplitter.cpp 3bandsplitter/*.cpp 3bandsplitter/*.h 3bandsplitter/*.hpp ../distrho/* $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -I../dgl -I3bandsplitter -DDISTRHO_NAMESPACE=DISTRHO_3BandSplitter -c -o $@

distrho-nekobi.cpp.o: distrho-nekobi.cpp nekobi/*.cpp nekobi/*.h nekobi/*.hpp nekobi/nekobee-src/*.c nekobi/nekobee-src/*.h ../distrho/* $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -I../dgl -Inekobi -DDISTRHO_NAMESPACE=DISTRHO_Nekobi -c -o $@

distrho-pingpongpan.cpp.o: distrho-pingpongpan.cpp pingpongpan/*.cpp pingpongpan/*.h pingpongpan/*.hpp ../distrho/* $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -I../dgl -Ipingpongpan -DDISTRHO_NAMESPACE=DISTRHO_PingPongPan -c -o $@

distrho-prom.cpp.o: distrho-prom.cpp prom/*.cpp prom/*.h prom/*.hpp ../distrho/* $(CXXDEPS)
	$(CXX) $< $(PM_CXX_FLAGS) -I../dgl -Iprom -DDISTRHO_NAMESPACE=DISTRHO_ProM -c -o $@

distrho-stereoenhancer.cpp.o: distrho-stereoenhancer.cpp stereoenhancer/*.cpp stereoenhancer/*.h stereoenhancer/*.hpp ../distrho/* $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -I../dgl -Istereoenhancer -DDISTRHO_NAMESPACE=DISTRHO_StereoEnhancer -c -o $@

distrho-groovejuice.cpp.o: distrho-groovejuice.cpp groovejuice/*.cpp groovejuice/*.h groovejuice/*.hpp groovejuice/*.hxx ../distrho/* $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -I../dgl -Igroovejuice -DDISTRHO_NAMESPACE=DISTRHO_GrooveJuice -c -o $@

distrho-powerjuice.cpp.o: distrho-powerjuice.cpp powerjuice/*.cpp powerjuice/*.h powerjuice/*.hpp ../distrho/* $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -I../dgl -Ipowerjuice -DDISTRHO_NAMESPACE=DISTRHO_PowerJuice -c -o $@

distrho-segmentjuice.cpp.o: distrho-segmentjuice.cpp segmentjuice/*.cpp segmentjuice/*.h segmentjuice/*.hpp segmentjuice/*.hxx ../distrho/* $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -I../dgl -Isegmentjuice -DDISTRHO_NAMESPACE=DISTRHO_SegmentJuice -c -o $@

distrho-vectorjuice.cpp.o: distrho-vectorjuice.cpp vectorjuice/*.cpp vectorjuice/*.h vectorjuice/*.hpp ../distrho/* $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -I../dgl -Ivectorjuice -DDISTRHO_NAMESPACE=DISTRHO_VectorJuice -c -o $@

distrho-wobblejuice.cpp.o: distrho-wobblejuice.cpp wobblejuice/*.cpp wobblejuice/*.h wobblejuice/*.hpp wobblejuice/*.hxx ../distrho/* $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -I../dgl -Iwobblejuice -DDISTRHO_NAMESPACE=DISTRHO_WobbleJuice -c -o $@

distrho-zamcomp.cpp.o: distrho-zamcomp.cpp zamcomp/*.cpp zamcomp/*.h zamcomp/*.hpp ../distrho/* $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -I../dgl -Izamcomp -DDISTRHO_NAMESPACE=DISTRHO_ZamComp -c -o $@

distrho-zamcompx2.cpp.o: distrho-zamcompx2.cpp zamcompx2/*.cpp zamcompx2/*.h zamcompx2/*.hpp ../distrho/* $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -I../dgl -Izamcompx2 -DDISTRHO_NAMESPACE=DISTRHO_ZamCompX2 -c -o $@

distrho-zameq2.cpp.o: distrho-zameq2.cpp zameq2/*.cpp zameq2/*.h zameq2/*.hpp ../distrho/* $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -I../dgl -Izameq2 -DDISTRHO_NAMESPACE=DISTRHO_ZamEQ2 -c -o $@

distrho-zamsynth.cpp.o: distrho-zamsynth.cpp zamsynth/*.cpp zamsynth/*.h zamsynth/*.hpp ../distrho/* $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -I../dgl -Izamsynth -DDISTRHO_NAMESPACE=DISTRHO_ZamSynth -c -o $@

distrho-zamtube.cpp.o: distrho-zamtube.cpp zamtube/*.cpp zamtube/*.h zamtube/*.hpp ../distrho/* $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -I../dgl -Izamtube -DDISTRHO_NAMESPACE=DISTRHO_ZamTube -c -o $@

distrho-zamulticomp.cpp.o: distrho-zamulticomp.cpp zamulticomp/*.cpp zamulticomp/*.h zamulticomp/*.hpp ../distrho/* $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -I../dgl -Izamulticomp -DDISTRHO_NAMESPACE=DISTRHO_ZaMultiComp -c -o $@

distrho-zamulticompx2.cpp.o: distrho-zamulticompx2.cpp zamulticompx2/*.cpp zamulticompx2/*.h zamulticompx2/*.hpp ../distrho/* $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -I../dgl -Izamulticompx2 -DDISTRHO_NAMESPACE=DISTRHO_ZaMultiCompX2 -c -o $@

juce-patchbay.cpp.o: juce-patchbay.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

midi-file.cpp.o: midi-file.cpp midi-base.hpp $(CXXDEPS)
	$(CXX) $< $(MF_CXX_FLAGS) -c -o $@

midi-sequencer.cpp.o: midi-sequencer.cpp midi-base.hpp $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

notes.cpp.o: notes.cpp $(CXXDEPS) ../CarlaNativeExtUI.hpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

vex-%.cpp.o: vex-%.cpp vex/*
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

zynaddsubfx-%.cpp.o: zynaddsubfx-%.cpp $(CXXDEPS) $(ZYN_UI_FILES_H)
	$(CXX) $< $(ZYN_CXX_FLAGS) -c -o $@

zynaddsubfx-src.cpp.o: zynaddsubfx-src.cpp $(ZYN_UI_FILES_H)
	$(CXX) $< $(ZYN_CXX_FLAGS) -c -o $@

zynaddsubfx-ui.cpp.o: zynaddsubfx-ui.cpp $(ZYN_UI_FILES_H) $(ZYN_UI_FILES_CPP)
	$(CXX) $< $(ZYN_CXX_FLAGS) -c -o $@

zynaddsubfx/UI/%.cpp: zynaddsubfx/UI/%.fl
	ntk-fluid -c -o zynaddsubfx/UI/$*.cpp -h zynaddsubfx/UI/$*.h $<

zynaddsubfx/UI/%.h: zynaddsubfx/UI/%.fl
	ntk-fluid -c -o zynaddsubfx/UI/$*.cpp -h zynaddsubfx/UI/$*.h $<

# --------------------------------------------------------------

clean:
	$(RM) *.o ../native-plugins*.a ../libnative-plugins.*
ifeq ($(HAVE_ZYN_UI_DEPS),true)
	$(RM) $(ZYN_UI_FILES_H) $(ZYN_UI_FILES_CPP)
endif

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
