#!/usr/bin/make -f
# Makefile for juce_graphics #
# -------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(JUCE_GRAPHICS_FLAGS) -I. -w

# --------------------------------------------------------------

ifeq ($(MACOS),true)
OBJS         = juce_graphics.mm.o
OBJS_posix32 = juce_graphics.mm.posix32.o
OBJS_posix64 = juce_graphics.mm.posix64.o
else
OBJS         = juce_graphics.cpp.o
OBJS_posix32 = juce_graphics.cpp.posix32.o
OBJS_posix64 = juce_graphics.cpp.posix64.o
endif
OBJS_win32   = juce_graphics.cpp.win32.o
OBJS_win64   = juce_graphics.cpp.win64.o

# --------------------------------------------------------------

all:     ../juce_graphics.a
posix32: ../juce_graphics.posix32.a
posix64: ../juce_graphics.posix64.a
win32:   ../juce_graphics.win32.a
win64:   ../juce_graphics.win64.a

# --------------------------------------------------------------

clean:
	$(RM) *.o ../juce_graphics*.a

debug:
	$(MAKE) DEBUG=true

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

# --------------------------------------------------------------

CPP_DEPS = juce_graphics.cpp juce_graphics.h AppConfig.h */*.cpp */*.h

juce_graphics.cpp.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

juce_graphics.cpp.%32.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

juce_graphics.cpp.%64.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

MM_DEPS = juce_graphics.cpp juce_graphics.h AppConfig.h */*.mm */*.cpp */*.h

juce_graphics.mm.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@

juce_graphics.mm.%32.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ $(32BIT_FLAGS) -c -o $@

juce_graphics.mm.%64.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
