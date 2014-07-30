#!/usr/bin/make -f
# Makefile for juce_audio_formats #
# ------------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(JUCE_AUDIO_FORMATS_FLAGS) -I. -w

# --------------------------------------------------------------

ifeq ($(MACOS),true)
OBJS         = juce_audio_formats.mm.o
OBJS_posix32 = juce_audio_formats.mm.posix32.o
OBJS_posix64 = juce_audio_formats.mm.posix64.o
else
OBJS         = juce_audio_formats.cpp.o
OBJS_posix32 = juce_audio_formats.cpp.posix32.o
OBJS_posix64 = juce_audio_formats.cpp.posix64.o
endif
OBJS_win32   = juce_audio_formats.cpp.win32.o
OBJS_win64   = juce_audio_formats.cpp.win64.o

# --------------------------------------------------------------

all:     ../juce_audio_formats.a
posix32: ../juce_audio_formats.posix32.a
posix64: ../juce_audio_formats.posix64.a
win32:   ../juce_audio_formats.win32.a
win64:   ../juce_audio_formats.win64.a

# --------------------------------------------------------------

clean:
	$(RM) *.o ../juce_audio_formats*.a

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

../juce_audio_formats.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

../juce_audio_formats.posix32.a: $(OBJS_posix32)
	$(RM) $@
	$(AR) crs $@ $^

../juce_audio_formats.posix64.a: $(OBJS_posix64)
	$(RM) $@
	$(AR) crs $@ $^

../juce_audio_formats.win32.a: $(OBJS_win32)
	$(RM) $@
	$(AR) crs $@ $^

../juce_audio_formats.win64.a: $(OBJS_win64)
	$(RM) $@
	$(AR) crs $@ $^

# --------------------------------------------------------------

CPP_DEPS = juce_audio_formats.cpp juce_audio_formats.h AppConfig.h */*.cpp */*.h

juce_audio_formats.cpp.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

juce_audio_formats.cpp.%32.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

juce_audio_formats.cpp.%64.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

MM_DEPS = juce_audio_formats.mm juce_audio_formats.h AppConfig.h */*.cpp */*.h

juce_audio_formats.mm.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@

juce_audio_formats.mm.%32.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ $(32BIT_FLAGS) -c -o $@

juce_audio_formats.mm.%64.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
