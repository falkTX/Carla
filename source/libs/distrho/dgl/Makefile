#!/usr/bin/make -f
# Makefile for dgl #
# ---------------- #
# Created by falkTX
#

include ../../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -fvisibility=hidden -fPIC
BUILD_CXX_FLAGS += -I.

OBJS   = \
	src/App.cpp.o \
	src/Image.cpp.o \
	src/ImageAboutWindow.cpp.o \
	src/ImageButton.cpp.o \
	src/ImageKnob.cpp.o \
	src/ImageSlider.cpp.o \
	src/Geometry.cpp.o \
	src/Widget.cpp.o \
	src/Window.cpp.o

STATIC = ../../dgl.a

# --------------------------------------------------------------

all: $(STATIC)

clean:
	rm -f $(OBJS) $(SHARED) $(STATIC)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

%.cpp.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(STATIC): $(OBJS)
	$(AR) rs $@ $^

../../dgl.dll: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -lopengl32 -lgdi32 -o $@

../../dgl.so: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -lX11 -lGL -lGLU -o $@
