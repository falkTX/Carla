#!/usr/bin/make -f
# Makefile for carla-native #
# ------------------------------------ #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_C_FLAGS   += -fvisibility=hidden -fPIC -I. -I.. -I../../includes
BUILD_CXX_FLAGS += -fvisibility=hidden -fPIC -I. -I.. -I../../includes -I../../utils -I../../libs/distrho-plugin-toolkit
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore QtGui)

LINK_FLAGS      += -shared
LINK_FLAGS      += $(shell pkg-config --libs QtCore QtGui) -lGL

ifeq ($(HAVE_ZYN_DEPS),true)
ZYN_CXX_FLAGS    = $(BUILD_CXX_FLAGS)
ZYN_CXX_FLAGS   += $(shell pkg-config --cflags fftw3 mxml)
LINK_FLAGS      += $(shell pkg-config --libs fftw3 mxml) -lpthread
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
	distrho-pingpongpan.cpp.o

# ZynAddSubFX
ifeq ($(HAVE_ZYN_DEPS),true)
OBJS += \
	zynaddsubfx.cpp.o \
	zynaddsubfx-src.cpp.o
endif

TARGET = ../carla_native.so

# --------------------------------------------------------------

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

# --------------------------------------------------------------

%.c.o: %.c
	$(CC) $< $(BUILD_C_FLAGS) -c -o $@

%.cpp.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(TARGET): $(OBJS)
	$(CXX) $^ $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

distrho-3bandeq.cpp.o: distrho-3bandeq.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -I3bandeq -DDISTRHO_NAMESPACE=DISTRHO_3BandEQ -c -o $@

distrho-3bandsplitter.cpp.o: distrho-3bandsplitter.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -I3bandsplitter -DDISTRHO_NAMESPACE=DISTRHO_3BandSplitter -c -o $@

distrho-pingpongpan.cpp.o: distrho-pingpongpan.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -Ipingpongpan -DDISTRHO_NAMESPACE=DISTRHO_PingPongPan -c -o $@

zynaddsubfx.cpp.o: zynaddsubfx.cpp
	$(CXX) $< $(ZYN_CXX_FLAGS) -c -o $@

zynaddsubfx-src.cpp.o: zynaddsubfx-src.cpp
	$(CXX) $< $(ZYN_CXX_FLAGS) -c -o $@
