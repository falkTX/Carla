#!/usr/bin/make -f
# Makefile for carla-native #
# ------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore QtGui) -I../../widgets
LINK_FLAGS      += $(shell pkg-config --libs QtCore QtGui)

ifeq ($(HAVE_AF_DEPS),true)
AF_C_FLAGS       = $(BUILD_C_FLAGS)
AF_C_FLAGS      += $(shell pkg-config --cflags libavcodec libavformat sndfile)
LINK_FLAGS      += $(shell pkg-config --libs libavcodec libavformat sndfile)
endif

ifeq ($(HAVE_ZYN_DEPS),true)
ZYN_CXX_FLAGS    = $(BUILD_CXX_FLAGS)
ZYN_CXX_FLAGS   += $(shell pkg-config --cflags fftw3 mxml zlib)
LINK_FLAGS      += $(shell pkg-config --libs fftw3 mxml zlib)
endif

# --------------------------------------------------------------

# Simple plugins
OBJS  = \
	bypass.c.o \
	midi-sequencer.cpp.o \
	midi-split.c.o \
	midi-through.c.o \
	midi-transpose.c.o \
	nekofilter.c.o

# DISTRHO plugins
OBJS += \
	distrho-3bandeq.cpp.o \
	distrho-3bandsplitter.cpp.o \
	distrho-pingpongpan.cpp.o \
	distrho-notes.cpp.o

# AudioFile
ifeq ($(HAVE_AF_DEPS),true)
OBJS += \
	audiofile.c.o
endif

# ZynAddSubFX
ifeq ($(HAVE_ZYN_DEPS),true)
OBJS += \
	zynaddsubfx.cpp.o
#	zynaddsubfx-src.cpp.o

OBJS += \
	zynaddsubfx/DSP/AnalogFilter.cpp.o \
	zynaddsubfx/DSP/FFTwrapper.cpp.o \
	zynaddsubfx/DSP/Filter.cpp.o \
	zynaddsubfx/DSP/FormantFilter.cpp.o \
	zynaddsubfx/DSP/SVFilter.cpp.o \
	zynaddsubfx/DSP/Unison.cpp.o \
	zynaddsubfx/Effects/Alienwah.cpp.o \
	zynaddsubfx/Effects/Chorus.cpp.o \
	zynaddsubfx/Effects/Distorsion.cpp.o \
	zynaddsubfx/Effects/DynamicFilter.cpp.o \
	zynaddsubfx/Effects/Echo.cpp.o \
	zynaddsubfx/Effects/Effect.cpp.o \
	zynaddsubfx/Effects/EffectLFO.cpp.o \
	zynaddsubfx/Effects/EffectMgr.cpp.o \
	zynaddsubfx/Effects/EQ.cpp.o \
	zynaddsubfx/Effects/Phaser.cpp.o \
	zynaddsubfx/Effects/Reverb.cpp.o \
	zynaddsubfx/Misc/Bank.cpp.o \
	zynaddsubfx/Misc/Config.cpp.o \
	zynaddsubfx/Misc/Dump.cpp.o \
	zynaddsubfx/Misc/Master.cpp.o \
	zynaddsubfx/Misc/Microtonal.cpp.o \
	zynaddsubfx/Misc/Part.cpp.o \
	zynaddsubfx/Misc/Recorder.cpp.o \
	zynaddsubfx/Misc/Util.cpp.o \
	zynaddsubfx/Misc/WavFile.cpp.o \
	zynaddsubfx/Misc/WaveShapeSmps.cpp.o \
	zynaddsubfx/Misc/XMLwrapper.cpp.o \
	zynaddsubfx/Params/ADnoteParameters.cpp.o \
	zynaddsubfx/Params/Controller.cpp.o \
	zynaddsubfx/Params/EnvelopeParams.cpp.o \
	zynaddsubfx/Params/FilterParams.cpp.o \
	zynaddsubfx/Params/LFOParams.cpp.o \
	zynaddsubfx/Params/PADnoteParameters.cpp.o \
	zynaddsubfx/Params/Presets.cpp.o \
	zynaddsubfx/Params/PresetsArray.cpp.o \
	zynaddsubfx/Params/PresetsStore.cpp.o \
	zynaddsubfx/Params/SUBnoteParameters.cpp.o \
	zynaddsubfx/Synth/ADnote.cpp.o \
	zynaddsubfx/Synth/Envelope.cpp.o \
	zynaddsubfx/Synth/LFO.cpp.o \
	zynaddsubfx/Synth/OscilGen.cpp.o \
	zynaddsubfx/Synth/PADnote.cpp.o \
	zynaddsubfx/Synth/Resonance.cpp.o \
	zynaddsubfx/Synth/SUBnote.cpp.o \
	zynaddsubfx/Synth/SynthNote.cpp.o
# 	zynaddsubfx/Misc/Stereo.cpp.o
endif

SHARED = ../libcarla_native.so
STATIC = ../libcarla_native.a

LIBS  = ../../libs/dgl.a
LIBS += ../../libs/widgets.a

# --------------------------------------------------------------

all: $(STATIC)

clean:
	rm -f $(OBJS) $(SHARED) $(STATIC)
	rm -f moc_*.cpp

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

%.c.o: %.c ../CarlaNative.h
	$(CC) $< $(BUILD_C_FLAGS) -c -o $@

%.cpp.o: %.cpp ../CarlaNative.h ../CarlaNative.hpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(SHARED): $(OBJS) $(LIBS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

$(STATIC): $(OBJS)
	$(AR) rs $@ $^

# --------------------------------------------------------------

distrho-3bandeq.cpp.o: distrho-3bandeq.cpp 3bandeq/DistrhoArtwork3BandEQ.cpp 3bandeq/DistrhoPlugin3BandEQ.cpp 3bandeq/DistrhoUI3BandEQ.cpp distrho/DistrhoPluginCarla.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -I3bandeq -DDISTRHO_NAMESPACE=DISTRHO_3BandEQ -c -o $@

distrho-3bandsplitter.cpp.o: distrho-3bandsplitter.cpp 3bandsplitter/DistrhoArtwork3BandSplitter.cpp 3bandsplitter/DistrhoPlugin3BandSplitter.cpp 3bandsplitter/DistrhoUI3BandSplitter.cpp distrho/DistrhoPluginCarla.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -I3bandsplitter -DDISTRHO_NAMESPACE=DISTRHO_3BandSplitter -c -o $@

distrho-pingpongpan.cpp.o: distrho-pingpongpan.cpp pingpongpan/DistrhoArtworkPingPongPan.cpp pingpongpan/DistrhoPluginPingPongPan.cpp pingpongpan/DistrhoUIPingPongPan.cpp distrho/DistrhoPluginCarla.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -Ipingpongpan -DDISTRHO_NAMESPACE=DISTRHO_PingPongPan -c -o $@

distrho-notes.cpp.o: distrho-notes.cpp notes/DistrhoPluginNotes.cpp notes/DistrhoUINotes.cpp distrho/DistrhoPluginCarla.cpp moc_DistrhoUINotes.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -Inotes -DDISTRHO_NAMESPACE=DISTRHO_Notes -c -o $@

audiofile.c.o: audiofile.c
	$(CC) $< $(AF_C_FLAGS) -c -o $@

zynaddsubfx.cpp.o: zynaddsubfx.cpp
	$(CXX) $< $(ZYN_CXX_FLAGS) -c -o $@

zynaddsubfx-src.cpp.o: zynaddsubfx-src.cpp
	$(CXX) $< $(ZYN_CXX_FLAGS) -c -o $@

zynaddsubfx/%.cpp.o: zynaddsubfx/%.cpp
	$(CXX) $< $(ZYN_CXX_FLAGS) -c -o $@

# --------------------------------------------------------------

moc_DistrhoUINotes.cpp: notes/DistrhoUINotes.hpp
	$(MOC) $< -o $@

# --------------------------------------------------------------

../libs/%:
	$(MAKE) -C ../libs $*
