#!/usr/bin/make -f
# Makefile for juce_audio_basics #
# ------------------------------ #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(JUCE_AUDIO_BASICS_FLAGS) -I.
LINK_FLAGS      += $(JUCE_AUDIO_BASICS_LIBS) -L.. -ljuce_core

ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -objc++
OBJS         = juce_audio_basics.mm.o
OBJS_posix32 = juce_audio_basics.mm.posix32.o
OBJS_posix64 = juce_audio_basics.mm.posix64.o
else
OBJS         = juce_audio_basics.cpp.o
OBJS_posix32 = juce_audio_basics.cpp.posix32.o
OBJS_posix64 = juce_audio_basics.cpp.posix64.o
endif

OBJS_win32 = juce_audio_basics.cpp.win32.o
OBJS_win64 = juce_audio_basics.cpp.win64.o

# --------------------------------------------------------------

all: ../juce_audio_basics.a

posix32: ../juce_audio_basics.posix32.a
posix64: ../juce_audio_basics.posix64.a
win32:   ../juce_audio_basics.win32.a
win64:   ../juce_audio_basics.win64.a

# --------------------------------------------------------------

../juce_audio_basics.a: $(OBJS)
	$(AR) rs $@ $^

../juce_audio_basics.posix32.a: $(OBJS_posix32)
	$(AR) rs $@ $^

../juce_audio_basics.posix64.a: $(OBJS_posix64)
	$(AR) rs $@ $^

../juce_audio_basics.win32.a: $(OBJS_win32)
	$(AR) rs $@ $^

../juce_audio_basics.win64.a: $(OBJS_win64)
	$(AR) rs $@ $^

../libjuce_audio_basics.dll: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

../libjuce_audio_basics.dylib: $(OBJS)
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -o $@

../libjuce_audio_basics.so: $(OBJS)
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
	rm -f *.o ../juce_audio_basics*.a ../libjuce_audio_basics.*

debug:
	$(MAKE) DEBUG=true
