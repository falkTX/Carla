#!/usr/bin/make -f
# Makefile for juce_audio_processors #
# ---------------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(JUCE_AUDIO_PROCESSORS_FLAGS) -I. -I../../includes/vst -I../../includes/vst3 -w
LINK_FLAGS      += $(JUCE_AUDIO_PROCESSORS_LIBS) -L.. -ljuce_audio_basics -ljuce_gui_basics -ljuce_graphics -ljuce_data_structures -ljuce_events -ljuce_core

ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -ObjC++
OBJS         = juce_audio_processors.mm.o
OBJS_posix32 = juce_audio_processors.mm.posix32.o
OBJS_posix64 = juce_audio_processors.mm.posix64.o
else
OBJS         = juce_audio_processors.cpp.o
OBJS_posix32 = juce_audio_processors.cpp.posix32.o
OBJS_posix64 = juce_audio_processors.cpp.posix64.o
endif

OBJS_win32 = juce_audio_processors.cpp.win32.o
OBJS_win64 = juce_audio_processors.cpp.win64.o

# --------------------------------------------------------------

all: ../juce_audio_processors.a

posix32: ../juce_audio_processors.posix32.a
posix64: ../juce_audio_processors.posix64.a
win32:   ../juce_audio_processors.win32.a
win64:   ../juce_audio_processors.win64.a

# --------------------------------------------------------------

../juce_audio_processors.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

../juce_audio_processors.posix32.a: $(OBJS_posix32)
	$(RM) $@
	$(AR) crs $@ $^

../juce_audio_processors.posix64.a: $(OBJS_posix64)
	$(RM) $@
	$(AR) crs $@ $^

../juce_audio_processors.win32.a: $(OBJS_win32)
	$(RM) $@
	$(AR) crs $@ $^

../juce_audio_processors.win64.a: $(OBJS_win64)
	$(RM) $@
	$(AR) crs $@ $^

../libjuce_audio_processors.dll: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

../libjuce_audio_processors.dylib: $(OBJS)
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -o $@

../libjuce_audio_processors.so: $(OBJS)
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
	$(RM) *.o ../juce_audio_processors*.a ../libjuce_audio_processors.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
