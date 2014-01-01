#!/usr/bin/make -f
# Makefile for juce_events #
# ------------------------ #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(JUCE_EVENTS_FLAGS) -I. -w
LINK_FLAGS      += $(JUCE_EVENTS_LIBS) -L.. -ljuce_core

ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -objc++
OBJS         = juce_events.mm.o
OBJS_posix32 = juce_events.mm.posix32.o
OBJS_posix64 = juce_events.mm.posix64.o
else
OBJS         = juce_events.cpp.o
OBJS_posix32 = juce_events.cpp.posix32.o
OBJS_posix64 = juce_events.cpp.posix64.o
endif

OBJS_win32 = juce_events.cpp.win32.o
OBJS_win64 = juce_events.cpp.win64.o

# --------------------------------------------------------------

all: ../juce_events.a

posix32: ../juce_events.posix32.a
posix64: ../juce_events.posix64.a
win32:   ../juce_events.win32.a
win64:   ../juce_events.win64.a

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

../libjuce_events.dll: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

../libjuce_events.dylib: $(OBJS)
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -o $@

../libjuce_events.so: $(OBJS)
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
	$(RM) *.o ../juce_events*.a ../libjuce_events.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
