#!/usr/bin/make -f
# Makefile for carla plugin exports #
# --------------------------------- #
# Created by falkTX
#

include ../Makefile.mk

BUILD_CXX_FLAGS += -I../backend -I../includes -I../modules -I../modules/utils

ifeq ($(HAVE_QT4),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore QtGui)
else
BUILD_CXX_FLAGS += $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Widgets)
endif

# --------------------------------------------------------------

ifeq ($(HAVE_OPENGL),true)
BUILD_CXX_FLAGS += -DWANT_OPENGL
endif

ifeq ($(HAVE_AF_DEPS),true)
BUILD_CXX_FLAGS += -DWANT_AUDIOFILE
endif

ifeq ($(HAVE_MF_DEPS),true)
BUILD_CXX_FLAGS += -DWANT_MIDIFILE
endif

ifeq ($(HAVE_ZYN_DEPS),true)
BUILD_CXX_FLAGS += -DWANT_ZYNADDSUBFX
endif

# --------------------------------------------------------------
# Common

LINK_FLAGS += $(shell pkg-config --libs liblo)

ifeq ($(HAVE_QT4),true)
LINK_FLAGS += $(shell pkg-config --libs QtCore QtGui QtXml)
else
LINK_FLAGS += $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Xml Qt5Widgets)
endif

# --------------------------------------------------------------
# Plugin

ifeq ($(HAVE_FLUIDSYNTH),true)
LINK_FLAGS += $(shell pkg-config --libs fluidsynth)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
LINK_FLAGS += $(shell pkg-config --libs linuxsampler)
endif

# --------------------------------------------------------------
# Native

DGL_LIBS = -lX11

ifeq ($(HAVE_AF_DEPS),true)
LINK_FLAGS += $(shell pkg-config --libs sndfile)
ifeq ($(HAVE_FFMPEG),true)
LINK_FLAGS += $(shell pkg-config --libs libavcodec libavformat libavutil)
endif
endif

ifeq ($(HAVE_MF_DEPS),true)
LINK_FLAGS += $(shell pkg-config --libs smf)
endif

ifeq ($(HAVE_OPENGL),true)
LINK_FLAGS += $(shell pkg-config --libs gl) $(DGL_LIBS)
endif

ifeq ($(HAVE_ZYN_DEPS),true)
LINK_FLAGS += $(shell pkg-config --libs fftw3 mxml zlib)
ifeq ($(HAVE_ZYN_UI_DEPS),true)
LINK_FLAGS += $(shell pkg-config --libs ntk ntk_images)
endif
endif

LINK_FLAGS += $(EXTRA_LIBS)

# --------------------------------------------------------------

# LIBS  = ../backend/libcarla_engine_xx.a
# LIBS += ../backend/libcarla_plugin.a
LIBS += ../backend/libcarla_native.a
LIBS += ../modules/rtmempool.a
LIBS += ../modules/widgets.a
LIBS += ../modules/juce_core.a

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
LIBS += ../modules/lilv.a
endif

ifeq ($(HAVE_OPENGL),true)
LIBS += ../modules/dgl.a
endif

ifeq ($(WIN32),true)
TARGETS  = carla-native.lv2/carla-native.dll
TARGETS += carla-native-export.exe
else
ifeq ($(MACOS),true)
TARGETS  = carla-native.lv2/carla-native.dylib
else
TARGETS  = carla-native.lv2/carla-native.so
endif
TARGETS += carla-native-export
endif

# --------------------------------------------------------------

all: $(TARGETS)

clean:
	rm -f *.o
	rm -f carla-native.lv2/*.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

%.cpp.o: %.cpp ../backend/CarlaNative.h
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

carla-native.lv2/carla-native.dll: carla-native-plugin.cpp.o $(LIBS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

carla-native.lv2/carla-native.dylib: carla-native-plugin.cpp.o $(LIBS)
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -o $@

carla-native.lv2/carla-native.so: carla-native-plugin.cpp.o $(LIBS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

carla-native-export: carla-native-export.cpp.o $(LIBS)
	$(CXX) $^ $(LINK_FLAGS) -o $@
	./carla-native-export

carla-native-export.exe: carla-native-export.cpp.o $(LIBS)
	$(CXX) $^ $(LINK_FLAGS) -o $@
	./carla-native-export.exe

# --------------------------------------------------------------

.FORCE:
.PHONY: .FORCE

../backend/libcarla_%.a: .FORCE
	$(MAKE) -C ../backend/$*

../modules/%.a: .FORCE
	$(MAKE) -C ../modules $*
