#!/usr/bin/make -f
# Makefile for dgl #
# ---------------- #
# Created by falkTX
#

include ../../../Makefile.mk

# --------------------------------------------------------------

BUILD_C_FLAGS   += $(DGL_FLAGS) -I. -w
BUILD_CXX_FLAGS += $(DGL_FLAGS) -I. -w
LINK_FLAGS      += $(DGL_LIBS)

OBJS = \
	src/App.cpp.o \
	src/Base.cpp.o \
	src/Image.cpp.o \
	src/ImageAboutWindow.cpp.o \
	src/ImageButton.cpp.o \
	src/ImageKnob.cpp.o \
	src/ImageSlider.cpp.o \
	src/Geometry.cpp.o \
	src/Widget.cpp.o \
	src/Window.cpp.o

OBJS_posix32 = \
	src/App.cpp.posix32.o \
	src/Base.cpp.posix32.o \
	src/Image.cpp.posix32.o \
	src/ImageAboutWindow.cpp.posix32.o \
	src/ImageButton.cpp.posix32.o \
	src/ImageKnob.cpp.posix32.o \
	src/ImageSlider.cpp.posix32.o \
	src/Geometry.cpp.posix32.o \
	src/Widget.cpp.posix32.o \
	src/Window.cpp.posix32.o

OBJS_posix64 = \
	src/App.cpp.posix64.o \
	src/Base.cpp.posix64.o \
	src/Image.cpp.posix64.o \
	src/ImageAboutWindow.cpp.posix64.o \
	src/ImageButton.cpp.posix64.o \
	src/ImageKnob.cpp.posix64.o \
	src/ImageSlider.cpp.posix64.o \
	src/Geometry.cpp.posix64.o \
	src/Widget.cpp.posix64.o \
	src/Window.cpp.posix64.o

OBJS_win32 = \
	src/App.cpp.win32.o \
	src/Base.cpp.win32.o \
	src/Image.cpp.win32.o \
	src/ImageAboutWindow.cpp.win32.o \
	src/ImageButton.cpp.win32.o \
	src/ImageKnob.cpp.win32.o \
	src/ImageSlider.cpp.win32.o \
	src/Geometry.cpp.win32.o \
	src/Widget.cpp.win32.o \
	src/Window.cpp.win32.o

OBJS_win64 = \
	src/App.cpp.win64.o \
	src/Base.cpp.win64.o \
	src/Image.cpp.win64.o \
	src/ImageAboutWindow.cpp.win64.o \
	src/ImageButton.cpp.win64.o \
	src/ImageKnob.cpp.win64.o \
	src/ImageSlider.cpp.win64.o \
	src/Geometry.cpp.win64.o \
	src/Widget.cpp.win64.o \
	src/Window.cpp.win64.o

ifeq ($(MACOS),true)
OBJS         += src/pugl/pugl_osx_extended.m.o
OBJS_posix32 += src/pugl/pugl_osx_extended.m.posix32.o
OBJS_posix64 += src/pugl/pugl_osx_extended.m.posix64.o
endif

# --------------------------------------------------------------

all: ../../dgl.a

posix32: ../../dgl.posix32.a
posix64: ../../dgl.posix64.a
win32:   ../../dgl.win32.a
win64:   ../../dgl.win64.a

# --------------------------------------------------------------

../../dgl.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

../../dgl.posix32.a: $(OBJS_posix32)
	$(RM) $@
	$(AR) crs $@ $^

../../dgl.posix64.a: $(OBJS_posix64)
	$(RM) $@
	$(AR) crs $@ $^

../../dgl.win32.a: $(OBJS_win32)
	$(RM) $@
	$(AR) crs $@ $^

../../dgl.win64.a: $(OBJS_win64)
	$(RM) $@
	$(AR) crs $@ $^

../../libdgl.dll: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

../../libdgl.dylib: $(OBJS)
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -o $@

../../libdgl.so: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

%.cpp.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

%.cpp.posix32.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

%.cpp.posix64.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

%.cpp.win32.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

%.cpp.win64.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

%.m.o: %.m
	$(CC) $< $(BUILD_C_FLAGS) -objc -c -o $@

# --------------------------------------------------------------

clean:
	$(RM) src/*.o src/pugl/*.o ../../dgl*.a ../../libdgl.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
