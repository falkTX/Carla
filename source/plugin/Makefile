#!/usr/bin/make -f
# Makefile for carla plugin exports #
# --------------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------
# Common

LINK_FLAGS += $(shell pkg-config --libs liblo)

ifeq ($(HAVE_QT5),true)
LINK_FLAGS += $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Xml Qt5Widgets)
else
LINK_FLAGS += $(shell pkg-config --libs QtCore QtGui QtXml)
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

LIBS  = ../backend/libcarla_engine_plugin.a
LIBS += ../backend/libcarla_plugin.a
LIBS += ../backend/libcarla_native.a
LIBS += ../libs/rtmempool.a
LIBS += ../libs/widgets.a

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
LIBS += ../libs/lilv.a
endif

ifeq ($(HAVE_OPENGL),true)
LIBS += ../libs/dgl.a
endif

# --------------------------------------------------------------

all: dssi vst
# all: dssi lv2 vst

dssi: carla-dssi.so
lv2:  carla-lv2.so
vst:  carla-vst.so

# --------------------------------------------------------------

carla-dssi.so: ../backend/engine/CarlaEnginePlugin.cpp.dssi.o $(LIBS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

carla-lv2.so: ../backend/engine/CarlaEnginePlugin.cpp.lv2.o $(LIBS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

carla-vst.so: ../backend/engine/CarlaEnginePlugin.cpp.vst.o $(LIBS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

../backend/engine/CarlaEnginePlugin.cpp.%.o: ../backend/engine/CarlaEnginePlugin.cpp
	make -C ../backend/engine CarlaEnginePlugin.cpp.$*.o
