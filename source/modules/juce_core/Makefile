#!/usr/bin/make -f
# Makefile for juce_core #
# ---------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(JUCE_CORE_FLAGS) -I. -w

# --------------------------------------------------------------

ifeq ($(MACOS),true)
OBJS         = juce_core.mm.o
OBJS_posix32 = juce_core.mm.posix32.o
OBJS_posix64 = juce_core.mm.posix64.o
else
OBJS         = juce_core.cpp.o
OBJS_posix32 = juce_core.cpp.posix32.o
OBJS_posix64 = juce_core.cpp.posix64.o
endif
OBJS_win32   = juce_core.cpp.win32.o
OBJS_win64   = juce_core.cpp.win64.o

# --------------------------------------------------------------

all:     ../juce_core.a
posix32: ../juce_core.posix32.a
posix64: ../juce_core.posix64.a
win32:   ../juce_core.win32.a
win64:   ../juce_core.win64.a

# --------------------------------------------------------------

clean:
	$(RM) *.o ../juce_core*.a

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

../juce_core.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

../juce_core.posix32.a: $(OBJS_posix32)
	$(RM) $@
	$(AR) crs $@ $^

../juce_core.posix64.a: $(OBJS_posix64)
	$(RM) $@
	$(AR) crs $@ $^

../juce_core.win32.a: $(OBJS_win32)
	$(RM) $@
	$(AR) crs $@ $^

../juce_core.win64.a: $(OBJS_win64)
	$(RM) $@
	$(AR) crs $@ $^

# --------------------------------------------------------------

CPP_DEPS = juce_core.cpp juce_core.h AppConfig.h */*.cpp */*.h

juce_core.cpp.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

juce_core.cpp.%32.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

juce_core.cpp.%64.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

MM_DEPS = juce_core.mm juce_core.h AppConfig.h */*.mm */*.cpp */*.h

juce_core.mm.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@

juce_core.mm.%32.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ $(32BIT_FLAGS) -c -o $@

juce_core.mm.%64.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
