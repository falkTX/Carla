#!/usr/bin/make -f
# Makefile for carla-native #
# ------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_C_FLAGS   += -I. -I.. -I../../includes
BUILD_CXX_FLAGS += -I. -I.. -I../../includes -I../../utils -I../distrho

LINK_FLAGS      += ../juce_audio_basics.a $(JUCE_AUDIO_BASICS_LIBS)
LINK_FLAGS      += ../juce_core.a $(JUCE_CORE_LIBS)
LINK_FLAGS      += ../juce_data_structures.a $(JUCE_DATA_STRUCTURES_LIBS)
LINK_FLAGS      += ../juce_events.a $(JUCE_EVENTS_LIBS)
LINK_FLAGS      += ../juce_graphics.a $(JUCE_GRAPHICS_LIBS)
LINK_FLAGS      += ../juce_gui_basics.a $(JUCE_GUI_BASICS_LIBS)

# --------------------------------------------------------------

ifeq ($(HAVE_OPENGL),true)
GL_CXX_FLAGS   = $(BUILD_CXX_FLAGS)
GL_CXX_FLAGS  += $(shell pkg-config --cflags gl)
LINK_FLAGS    += ../dgl.a $(shell pkg-config --libs gl)
endif

ifeq ($(HAVE_AF_DEPS),true)
AF_C_FLAGS     = $(BUILD_C_FLAGS)
AF_C_FLAGS    += $(shell pkg-config --cflags sndfile)
LINK_FLAGS    += $(shell pkg-config --libs sndfile)
ifeq ($(HAVE_FFMPEG),true)
AF_C_FLAGS    += -DHAVE_FFMPEG
AF_C_FLAGS    += $(shell pkg-config --cflags libavcodec libavformat libavutil)
LINK_FLAGS    += $(shell pkg-config --libs libavcodec libavformat libavutil)
endif
endif

ifeq ($(HAVE_MF_DEPS),true)
MF_CXX_FLAGS   = $(BUILD_CXX_FLAGS)
MF_CXX_FLAGS  += $(shell pkg-config --cflags smf)
LINK_FLAGS    += $(shell pkg-config --libs smf)
endif

ifeq ($(HAVE_ZYN_DEPS),true)
ZYN_CXX_FLAGS  = $(BUILD_CXX_FLAGS)
ZYN_CXX_FLAGS += $(shell pkg-config --cflags fftw3 mxml zlib)
LINK_FLAGS    += $(shell pkg-config --libs fftw3 mxml zlib)
ifeq ($(HAVE_ZYN_UI_DEPS),true)
ZYN_CXX_FLAGS += -DNTK_GUI -DWANT_ZYNADDSUBFX_UI
ZYN_CXX_FLAGS += $(shell pkg-config --cflags ntk_images ntk)
LINK_FLAGS    += $(shell pkg-config --libs ntk_images ntk)
endif
endif

# Implement carla_register_native_plugin
LINK_FLAGS += -I.. _link.c

# --------------------------------------------------------------

# Simple plugins (C)
OBJS  = \
	bypass.c.o \
	lfo.c.o \
	midi-gain.c.o \
	midi-split.c.o \
	midi-through.c.o \
	midi-transpose.c.o \
	nekofilter.c.o

# Simple plugins (C++)
OBJS += \
	vex-fx.cpp.o \
	vex-synth.cpp.o \
	vex-src.cpp.o

# AudioFile
ifeq ($(HAVE_AF_DEPS),true)
OBJS += \
	audio-file.cpp.o \
	audio_decoder/ad_ffmpeg.c.o \
	audio_decoder/ad_plugin.c.o \
	audio_decoder/ad_soundfile.c.o
endif

# MidiFile
ifeq ($(HAVE_MF_DEPS),true)
OBJS += \
	midi-file.cpp.o
endif

# DISTRHO plugins (OpenGL)
ifeq ($(HAVE_OPENGL),true)
OBJS += \
	distrho-3bandeq.cpp.o \
	distrho-3bandsplitter.cpp.o \
	distrho-nekobi.cpp.o \
	distrho-pingpongpan.cpp.o
# 	distrho-stereoenhancer.cpp.o
endif

# DISTRHO plugins (PyQt)
OBJS += \
	distrho-bigmeter.cpp.o \
	distrho-bigmeterM.cpp.o \
	distrho-notes.cpp.o

ifeq ($(HAVE_ZYN_DEPS),true)
# ZynAddSubFX
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

all: ../carla_native.a

# --------------------------------------------------------------

../carla_native.a: $(OBJS)
	rm -f $@
	$(AR) rs $@ $^

../libcarla_native.dll: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

../libcarla_native.dylib: $(OBJS)
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -o $@

../libcarla_native.so: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

CDEPS   = ../CarlaNative.h
CXXDEPS = ../CarlaNative.h ../CarlaNative.hpp

audio_decoder/%.c.o: audio_decoder/%.c
	$(CC) $< $(AF_C_FLAGS) -c -o $@

audio-file.cpp.o: audio-file.cpp audio-base.hpp $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

distrho-3bandeq.cpp.o: distrho-3bandeq.cpp 3bandeq/*.cpp 3bandeq/*.h 3bandeq/*.hpp distrho/DistrhoPluginCarla.cpp $(CXXDEPS)
	$(CXX) $< $(GL_CXX_FLAGS) -I3bandeq -DDISTRHO_NAMESPACE=DISTRHO_3BandEQ -c -o $@

distrho-3bandsplitter.cpp.o: distrho-3bandsplitter.cpp 3bandsplitter/*.cpp 3bandsplitter/*.h 3bandsplitter/*.hpp distrho/DistrhoPluginCarla.cpp $(CXXDEPS)
	$(CXX) $< $(GL_CXX_FLAGS) -I3bandsplitter -DDISTRHO_NAMESPACE=DISTRHO_3BandSplitter -c -o $@

distrho-nekobi.cpp.o: distrho-nekobi.cpp nekobi/*.cpp nekobi/*.h nekobi/*.hpp nekobi/nekobee-src/*.c nekobi/nekobee-src/*.h distrho/DistrhoPluginCarla.cpp $(CXXDEPS)
	$(CXX) $< $(GL_CXX_FLAGS) -Inekobi -DDISTRHO_NAMESPACE=DISTRHO_Nekobi -c -o $@

distrho-pingpongpan.cpp.o: distrho-pingpongpan.cpp pingpongpan/*.cpp pingpongpan/*.h pingpongpan/*.hpp distrho/DistrhoPluginCarla.cpp $(CXXDEPS)
	$(CXX) $< $(GL_CXX_FLAGS) -Ipingpongpan -DDISTRHO_NAMESPACE=DISTRHO_PingPongPan -c -o $@

distrho-stereoenhancer.cpp.o: distrho-stereoenhancer.cpp stereoenhancer/*.cpp stereoenhancer/*.h stereoenhancer/*.hpp distrho/DistrhoPluginCarla.cpp $(CXXDEPS)
	$(CXX) $< $(GL_CXX_FLAGS) -Istereoenhancer -DDISTRHO_NAMESPACE=DISTRHO_StereoEnhancer -c -o $@

distrho-bigmeter.cpp.o: distrho-bigmeter.cpp bigmeter/*.cpp bigmeter/*.h bigmeter/*.hpp distrho/DistrhoPluginCarla.cpp $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -Ibigmeter -DDISTRHO_NAMESPACE=DISTRHO_BigMeter -c -o $@

distrho-bigmeterM.cpp.o: distrho-bigmeterM.cpp bigmeterM/*.cpp bigmeterM/*.h bigmeterM/*.hpp distrho/DistrhoPluginCarla.cpp $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -IbigmeterM -DDISTRHO_NAMESPACE=DISTRHO_BigMeterM -c -o $@

distrho-notes.cpp.o: distrho-notes.cpp notes/*.cpp notes/*.h notes/*.hpp distrho/DistrhoPluginCarla.cpp $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -Inotes -DDISTRHO_NAMESPACE=DISTRHO_Notes -c -o $@

midi-file.cpp.o: midi-file.cpp midi-base.hpp $(CXXDEPS)
	$(CXX) $< $(MF_CXX_FLAGS) -c -o $@

midi-sequencer.cpp.o: midi-sequencer.cpp midi-base.hpp $(CXXDEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

nekofilter.c.o: nekofilter.c nekofilter/*.c nekofilter/*.h $(CDEPS)
	$(CC) $< $(BUILD_C_FLAGS) -c -o $@

zynaddsubfx-ui.cpp.o: zynaddsubfx-ui.cpp $(ZYN_UI_FILES_H) $(ZYN_UI_FILES_CPP)
	$(CXX) $< $(ZYN_CXX_FLAGS) -c -o $@

zynaddsubfx-%.cpp.o: zynaddsubfx-%.cpp $(CXXDEPS) $(ZYN_UI_FILES_H)
	$(CXX) $< $(ZYN_CXX_FLAGS) -c -o $@

zynaddsubfx/UI/%.cpp: zynaddsubfx/UI/%.fl
	ntk-fluid -c -o zynaddsubfx/UI/$*.cpp -h zynaddsubfx/UI/$*.h $<

zynaddsubfx/UI/%.h: zynaddsubfx/UI/%.fl
	ntk-fluid -c -o zynaddsubfx/UI/$*.cpp -h zynaddsubfx/UI/$*.h $<

# --------------------------------------------------------------

%.c.o: %.c
	$(CC) $< $(BUILD_C_FLAGS) -c -o $@

# %.cpp.o: %.cpp
# 	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

moc_%.cpp: %.hpp
	$(MOC) $< -DMOC_PARSING -o $@

# --------------------------------------------------------------

clean:
	rm -f $(OBJS) ../carla_native*.a ../libcarla_native.*
	rm -f $(ZYN_UI_FILES_H) $(ZYN_UI_FILES_CPP)

debug:
	$(MAKE) DEBUG=true
