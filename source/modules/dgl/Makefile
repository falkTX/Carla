#!/usr/bin/make -f
# Makefile for dgl #
# ---------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(DGL_FLAGS) -I.

# --------------------------------------------------------------

OBJS = \
	src/App.cpp.o \
	src/Geometry.cpp.o \
	src/Image.cpp.o \
	src/ImageAboutWindow.cpp.o \
	src/ImageButton.cpp.o \
	src/ImageKnob.cpp.o \
	src/ImageSlider.cpp.o \
	src/ImageSwitch.cpp.o \
	src/NanoVG.cpp.o \
	src/Widget.cpp.o

ifeq ($(MACOS),true)
OBJS += src/Window.mm.o
else
OBJS += src/Window.cpp.o
endif

TARGETS = \
	../dgl.a

# --------------------------------------------------------------

all: $(TARGETS)

# --------------------------------------------------------------

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

../dgl.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

# --------------------------------------------------------------

BASE_HPP     = Base.hpp ../distrho/extra/d_leakdetector.hpp ../distrho/extra/d_scopedpointer.hpp ../distrho/DistrhoUtils.hpp ../distrho/src/DistrhoDefines.h
APP_PRIV_HPP = src/AppPrivateData.hpp App.hpp ../distrho/extra/d_sleep.hpp $(BASE_HPP)
GEOMETRY_HPP = Geometry.hpp $(BASE_HPP)
IMAGE_HPP    = Image.hpp $(GEOMETRY_HPP)
WIDGET_HPP   = Widget.hpp $(GEOMETRY_HPP)
WINDOW_HPP   = Window.hpp $(GEOMETRY_HPP)

src/App.cpp.o: src/App.cpp $(APP_PRIV_HPP) $(WINDOW_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

src/Geometry.cpp.o: src/Geometry.cpp $(GEOMETRY_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

src/Image.cpp.o: src/Image.cpp $(IMAGE_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

src/ImageAboutWindow.cpp.o: src/ImageAboutWindow.cpp ImageAboutWindow.hpp $(IMAGE_HPP) $(WIDGET_HPP) $(WINDOW_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

src/ImageButton.cpp.o: src/ImageButton.cpp ImageButton.hpp $(IMAGE_HPP) $(WIDGET_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

src/ImageKnob.cpp.o: src/ImageKnob.cpp ImageKnob.hpp $(IMAGE_HPP) $(WIDGET_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

src/ImageSlider.cpp.o: src/ImageSlider.cpp ImageSlider.hpp $(IMAGE_HPP) $(WIDGET_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

src/ImageSwitch.cpp.o: src/ImageSwitch.cpp ImageSwitch.hpp $(IMAGE_HPP) $(WIDGET_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

src/NanoVG.cpp.o: src/NanoVG.cpp NanoVG.hpp $(WIDGET_HPP) $(WINDOW_HPP) src/nanovg/*
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

src/Widget.cpp.o: src/Widget.cpp $(WIDGET_HPP) $(WINDOW_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

src/Window.cpp.o: src/Window.cpp $(APP_PRIV_HPP) $(WIDGET_HPP) $(WINDOW_HPP) src/pugl/*
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

src/Window.mm.o: src/Window.mm src/Window.cpp $(APP_PRIV_HPP) $(WIDGET_HPP) $(WINDOW_HPP) src/pugl/*
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@

# --------------------------------------------------------------
