#!/usr/bin/make -f
# Makefile for juce_audio_devices #
# ------------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(JUCE_AUDIO_DEVICES_FLAGS) -I. -w
LINK_FLAGS      += $(JUCE_AUDIO_DEVICES_LIBS) -L.. -ljuce_events -ljuce_audio_formats -ljuce_audio_basics -ljuce_core

ifeq ($(WIN32),true)
BUILD_CXX_FLAGS += -I../../includes/asio
endif

ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -ObjC++
OBJS         = juce_audio_devices.mm.o
OBJS_posix32 = juce_audio_devices.mm.posix32.o
OBJS_posix64 = juce_audio_devices.mm.posix64.o
else
OBJS         = juce_audio_devices.cpp.o
OBJS_posix32 = juce_audio_devices.cpp.posix32.o
OBJS_posix64 = juce_audio_devices.cpp.posix64.o
endif

OBJS_win32 = juce_audio_devices.cpp.win32.o
OBJS_win64 = juce_audio_devices.cpp.win64.o

# --------------------------------------------------------------

all: ../juce_audio_devices.a

posix32: ../juce_audio_devices.posix32.a
posix64: ../juce_audio_devices.posix64.a
win32:   ../juce_audio_devices.win32.a
win64:   ../juce_audio_devices.win64.a

# --------------------------------------------------------------

../juce_audio_devices.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

../juce_audio_devices.posix32.a: $(OBJS_posix32)
	$(RM) $@
	$(AR) crs $@ $^

../juce_audio_devices.posix64.a: $(OBJS_posix64)
	$(RM) $@
	$(AR) crs $@ $^

../juce_audio_devices.win32.a: $(OBJS_win32)
	$(RM) $@
	$(AR) crs $@ $^

../juce_audio_devices.win64.a: $(OBJS_win64)
	$(RM) $@
	$(AR) crs $@ $^

../libjuce_audio_devices.dll: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

../libjuce_audio_devices.dylib: $(OBJS)
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -o $@

../libjuce_audio_devices.so: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

%.cpp.o: %.cpp *.h */*.cpp */*.h
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

%.mm.o: %.mm %.cpp *.h */*.cpp */*.h */*.mm
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
	$(RM) *.o ../juce_audio_devices*.a ../libjuce_audio_devices.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
