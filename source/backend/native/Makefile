#!/usr/bin/make -f
# Makefile for carla-native #
# ------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../../libs/distrho
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore QtGui gl)
LINK_FLAGS      += $(shell pkg-config --libs QtCore QtGui gl)

ifeq ($(HAVE_AF_DEPS),true)
AF_CXX_FLAGS     = $(BUILD_CXX_FLAGS)
AF_CXX_FLAGS    += $(shell pkg-config --cflags libavcodec libavformat sndfile)
LINK_FLAGS      += $(shell pkg-config --libs libavcodec libavformat sndfile)
endif

ifeq ($(HAVE_ZYN_DEPS),true)
ZYN_CXX_FLAGS    = $(BUILD_CXX_FLAGS)
ZYN_CXX_FLAGS   += $(shell pkg-config --cflags fftw3 mxml)
LINK_FLAGS      += $(shell pkg-config --libs fftw3 mxml)
endif

# --------------------------------------------------------------

# Simple plugins
OBJS  = \
	bypass.c.o \
	midi-split.c.o \
	midi-through.c.o

# DISTRHO plugins
OBJS += \
	distrho-3bandeq.cpp.o \
	distrho-3bandsplitter.cpp.o \
	distrho-pingpongpan.cpp.o \
	distrho-notes.cpp.o

ifeq ($(HAVE_AF_DEPS),true)
OBJS += \
	audiofile.c.o
endif

# ZynAddSubFX
ifeq ($(HAVE_ZYN_DEPS),true)
OBJS += \
	zynaddsubfx.cpp.o \
	zynaddsubfx-src.cpp.o
endif

SHARED = ../libcarla_native.so
STATIC = ../libcarla_native.a

LIBS = ../../libs/dgl.a

# --------------------------------------------------------------

all: $(STATIC)

clean:
	rm -f $(OBJS) $(SHARED) $(STATIC)
	rm -f moc_*.cpp

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

%.c.o: %.c
	$(CC) $< $(BUILD_C_FLAGS) -c -o $@

%.cpp.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(SHARED): $(OBJS) $(LIBS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

$(STATIC): $(OBJS)
	$(AR) rs $@ $^

# --------------------------------------------------------------

distrho-3bandeq.cpp.o: distrho-3bandeq.cpp 3bandeq/DistrhoArtwork3BandEQ.cpp 3bandeq/DistrhoPlugin3BandEQ.cpp 3bandeq/DistrhoUI3BandEQ.cpp distrho/DistrhoPluginCarla.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -I3bandeq -DDISTRHO_NAMESPACE=DISTRHO_3BandEQ -c -o $@

distrho-3bandsplitter.cpp.o: distrho-3bandsplitter.cpp distrho/DistrhoPluginCarla.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -I3bandsplitter -DDISTRHO_NAMESPACE=DISTRHO_3BandSplitter -c -o $@

distrho-pingpongpan.cpp.o: distrho-pingpongpan.cpp distrho/DistrhoPluginCarla.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -Ipingpongpan -DDISTRHO_NAMESPACE=DISTRHO_PingPongPan -c -o $@

distrho-notes.cpp.o: distrho-notes.cpp distrho/DistrhoPluginCarla.cpp moc_DistrhoUINotes.cpp moc_ParamProgressBar.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -Inotes -DDISTRHO_NAMESPACE=DISTRHO_Notes -c -o $@

zynaddsubfx.cpp.o: zynaddsubfx.cpp
	$(CXX) $< $(ZYN_CXX_FLAGS) -c -o $@

zynaddsubfx-src.cpp.o: zynaddsubfx-src.cpp
	$(CXX) $< $(ZYN_CXX_FLAGS) -c -o $@

# --------------------------------------------------------------

moc_DistrhoUINotes.cpp: notes/DistrhoUINotes.hpp
	$(MOC) $< -o $@

moc_ParamProgressBar.cpp: notes/ParamProgressBar.hpp
	$(MOC) $< -o $@
