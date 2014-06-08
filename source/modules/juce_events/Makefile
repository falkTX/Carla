#!/usr/bin/make -f
# Makefile for juce_events #
# ------------------------ #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(JUCE_EVENTS_FLAGS) -I. -w

# --------------------------------------------------------------

ifeq ($(MACOS),true)
OBJS         = juce_events.mm.o
OBJS_posix32 = juce_events.mm.posix32.o
OBJS_posix64 = juce_events.mm.posix64.o
else
OBJS         = juce_events.cpp.o
OBJS_posix32 = juce_events.cpp.posix32.o
OBJS_posix64 = juce_events.cpp.posix64.o
endif
OBJS_win32   = juce_events.cpp.win32.o
OBJS_win64   = juce_events.cpp.win64.o

# --------------------------------------------------------------

all:     ../juce_events.a
posix32: ../juce_events.posix32.a
posix64: ../juce_events.posix64.a
win32:   ../juce_events.win32.a
win64:   ../juce_events.win64.a

# --------------------------------------------------------------

clean:
	$(RM) *.o ../juce_events*.a

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

../juce_events.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

../juce_events.posix32.a: $(OBJS_posix32)
	$(RM) $@
	$(AR) crs $@ $^

../juce_events.posix64.a: $(OBJS_posix64)
	$(RM) $@
	$(AR) crs $@ $^

../juce_events.win32.a: $(OBJS_win32)
	$(RM) $@
	$(AR) crs $@ $^

../juce_events.win64.a: $(OBJS_win64)
	$(RM) $@
	$(AR) crs $@ $^

# --------------------------------------------------------------

CPP_DEPS = juce_events.cpp juce_events.h AppConfig.h */*.cpp */*.h

juce_events.cpp.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

juce_events.cpp.%32.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

juce_events.cpp.%64.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

MM_DEPS = juce_events.mm juce_events.h AppConfig.h */*.mm */*.cpp */*.h

juce_events.mm.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@

juce_events.mm.%32.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ $(32BIT_FLAGS) -c -o $@

juce_events.mm.%64.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
