#!/usr/bin/make -f
# Makefile for carla-native #
# ------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I. -I../../libs/distrho -I../../widgets

ifeq ($(HAVE_QT5),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Widgets)
else
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore QtGui)
endif

# --------------------------------------------------------------

CARLA_CXX_FLAGS  = $(BUILD_CXX_FLAGS)
CARLA_CXX_FLAGS += $(shell pkg-config --cflags liblo)
CARLA_CXX_FLAGS += -I../engine

ifeq ($(HAVE_AF_DEPS),true)
AF_C_FLAGS       = $(BUILD_C_FLAGS)
AF_C_FLAGS      += $(shell pkg-config --cflags sndfile)
ifeq ($(HAVE_FFMPEG),true)
AF_C_FLAGS      += -DHAVE_FFMPEG
AF_C_FLAGS      += $(shell pkg-config --cflags libavcodec libavformat libavutil)
endif
endif

ifeq ($(HAVE_MF_DEPS),true)
MF_CXX_FLAGS     = $(BUILD_CXX_FLAGS)
MF_CXX_FLAGS    += $(shell pkg-config --cflags smf)
endif

ifeq ($(HAVE_OPENGL),true)
GL_CXX_FLAGS     = $(BUILD_CXX_FLAGS)
GL_CXX_FLAGS    += $(shell pkg-config --cflags gl)
endif

ifeq ($(HAVE_ZYN_DEPS),true)
ZYN_CXX_FLAGS    = $(BUILD_CXX_FLAGS)
ZYN_CXX_FLAGS   += $(shell pkg-config --cflags fftw3 mxml zlib)
ifeq ($(HAVE_ZYN_UI_DEPS),true)
ZYN_CXX_FLAGS   += -DNTK_GUI
ZYN_CXX_FLAGS   += $(shell pkg-config --cflags ntk ntk_images)
endif
endif

# --------------------------------------------------------------

# Simple plugins
OBJS  = \
	bypass.c.o \
	lfo.c.o \
	midi-sequencer.cpp.o \
	midi-split.c.o \
	midi-through.c.o \
	midi-transpose.c.o \
	nekofilter.c.o \
	sunvox-file.cpp.o

# Carla
OBJS += \
	carla.cpp.o

# AudioFile
ifeq ($(HAVE_AF_DEPS),true)
OBJS += \
	audiofile.c.o
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
	distrho-pingpongpan.cpp.o
endif

# DISTRHO plugins (Qt)
OBJS += \
	distrho-notes.cpp.o

ifeq ($(HAVE_ZYN_DEPS),true)
# ZynAddSubFX
OBJS += \
	zynaddsubfx.cpp.o \
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

TARGET = ../libcarla_native.a

# --------------------------------------------------------------

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
	rm -f $(ZYN_UI_FILES_CPP)
	rm -f $(ZYN_UI_FILES_H)
	rm -f moc_*.cpp

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

%.c.o: %.c ../CarlaNative.h
	$(CC) $< $(BUILD_C_FLAGS) -c -o $@

%.cpp.o: %.cpp ../CarlaNative.h ../CarlaNative.hpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(TARGET): $(OBJS)
	$(AR) rs $@ $^

# --------------------------------------------------------------

audiofile.c.o: audiofile.c
	$(CC) $< $(AF_C_FLAGS) -c -o $@

carla.cpp.o: carla.cpp
	$(CXX) $< $(CARLA_CXX_FLAGS) -c -o $@

distrho-3bandeq.cpp.o: distrho-3bandeq.cpp 3bandeq/DistrhoArtwork3BandEQ.cpp 3bandeq/DistrhoPlugin3BandEQ.cpp 3bandeq/DistrhoUI3BandEQ.cpp distrho/DistrhoPluginCarla.cpp
	$(CXX) $< $(GL_CXX_FLAGS) -I3bandeq -DDISTRHO_NAMESPACE=DISTRHO_3BandEQ -c -o $@

distrho-3bandsplitter.cpp.o: distrho-3bandsplitter.cpp 3bandsplitter/DistrhoArtwork3BandSplitter.cpp 3bandsplitter/DistrhoPlugin3BandSplitter.cpp 3bandsplitter/DistrhoUI3BandSplitter.cpp distrho/DistrhoPluginCarla.cpp
	$(CXX) $< $(GL_CXX_FLAGS) -I3bandsplitter -DDISTRHO_NAMESPACE=DISTRHO_3BandSplitter -c -o $@

distrho-pingpongpan.cpp.o: distrho-pingpongpan.cpp pingpongpan/DistrhoArtworkPingPongPan.cpp pingpongpan/DistrhoPluginPingPongPan.cpp pingpongpan/DistrhoUIPingPongPan.cpp distrho/DistrhoPluginCarla.cpp
	$(CXX) $< $(GL_CXX_FLAGS) -Ipingpongpan -DDISTRHO_NAMESPACE=DISTRHO_PingPongPan -c -o $@

distrho-notes.cpp.o: distrho-notes.cpp notes/DistrhoPluginNotes.cpp notes/DistrhoUINotes.cpp distrho/DistrhoPluginCarla.cpp moc_DistrhoUINotes.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -Inotes -DDISTRHO_NAMESPACE=DISTRHO_Notes -c -o $@

midi-file.cpp.o: midi-file.cpp
	$(CXX) $< $(MF_CXX_FLAGS) -c -o $@

zynaddsubfx.cpp.o: zynaddsubfx.cpp $(ZYN_UI_FILES_H)
	$(CXX) $< $(ZYN_CXX_FLAGS) -c -o $@

zynaddsubfx-src.cpp.o: zynaddsubfx-src.cpp
	$(CXX) $< $(ZYN_CXX_FLAGS) -c -o $@

zynaddsubfx-ui.cpp.o: zynaddsubfx-ui.cpp $(ZYN_UI_FILES_CPP)
	$(CXX) $< $(ZYN_CXX_FLAGS) -c -o $@

# --------------------------------------------------------------

moc_DistrhoUINotes.cpp: notes/DistrhoUINotes.hpp
	$(MOC) $< -o $@

zynaddsubfx/UI/%.cpp: zynaddsubfx/UI/%.fl
	ntk-fluid -c -o zynaddsubfx/UI/$*.cpp -h zynaddsubfx/UI/$*.h $<

zynaddsubfx/UI/%.h: zynaddsubfx/UI/%.fl
	ntk-fluid -c -o zynaddsubfx/UI/$*.cpp -h zynaddsubfx/UI/$*.h $<

# --------------------------------------------------------------

../libs/%:
	$(MAKE) -C ../libs $*
