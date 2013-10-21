#!/usr/bin/make -f
# Makefile for juce_graphics #
# -------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(JUCE_GRAPHICS_FLAGS) -I.
LINK_FLAGS      += $(JUCE_GRAPHICS_LIBS) -L.. -ljuce_events -ljuce_core

ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -objc++
OBJS         = juce_graphics.mm.o
OBJS_posix32 = juce_graphics.mm.posix32.o
OBJS_posix64 = juce_graphics.mm.posix64.o
else
OBJS         = juce_graphics.cpp.o
OBJS_posix32 = juce_graphics.cpp.posix32.o
OBJS_posix64 = juce_graphics.cpp.posix64.o
endif

OBJS_win32 = juce_graphics.cpp.win32.o
OBJS_win64 = juce_graphics.cpp.win64.o

# --------------------------------------------------------------

all: ../juce_graphics.a

posix32: ../juce_graphics.posix32.a
posix64: ../juce_graphics.posix64.a
win32:   ../juce_graphics.win32.a
win64:   ../juce_graphics.win64.a

# --------------------------------------------------------------

../juce_graphics.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

../juce_graphics.posix32.a: $(OBJS_posix32)
	$(RM) $@
	$(AR) crs $@ $^

../juce_graphics.posix64.a: $(OBJS_posix64)
	$(RM) $@
	$(AR) crs $@ $^

../juce_graphics.win32.a: $(OBJS_win32)
	$(RM) $@
	$(AR) crs $@ $^

../juce_graphics.win64.a: $(OBJS_win64)
	$(RM) $@
	$(AR) crs $@ $^

../libjuce_graphics.dll: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

../libjuce_graphics.dylib: $(OBJS)
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -o $@

../libjuce_graphics.so: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

%.cpp.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

%.mm.o: %.mm
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

%.posix32.o: %
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

%.posix64.o: %
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

%.win32.o: %
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

%.win64.o: %
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

clean:
	$(RM) *.o ../juce_graphics*.a ../libjuce_graphics.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
