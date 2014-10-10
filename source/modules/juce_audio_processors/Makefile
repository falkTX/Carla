#!/usr/bin/make -f
# Makefile for juce_audio_processors #
# ---------------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(JUCE_AUDIO_PROCESSORS_FLAGS) -I. -I../../includes/ladspa -I../../includes/vst -I../../includes/vst3 -w

ifeq ($(CARLA_VESTIGE_HEADER),true)
BUILD_CXX_FLAGS += -DVESTIGE_HEADER
else
# needed by vst3
ifeq ($(DEBUG),true)
BUILD_CXX_FLAGS += -DDEVELOPMENT -D_DEBUG
else
BUILD_CXX_FLAGS += -DRELEASE
endif
# needed by vst3 on mingw
ifeq ($(WIN32),true)
BUILD_CXX_FLAGS += -D_NATIVE_WCHAR_T_DEFINED -D__wchar_t=wchar_t -fpermissive
endif
endif

# --------------------------------------------------------------

ifeq ($(MACOS),true)
OBJS         = juce_audio_processors.mm.o
OBJS_posix32 = juce_audio_processors.mm.posix32.o
OBJS_posix64 = juce_audio_processors.mm.posix64.o
else
OBJS         = juce_audio_processors.cpp.o
OBJS_posix32 = juce_audio_processors.cpp.posix32.o
OBJS_posix64 = juce_audio_processors.cpp.posix64.o
endif
OBJS_win32   = juce_audio_processors.cpp.win32.o
OBJS_win64   = juce_audio_processors.cpp.win64.o

# --------------------------------------------------------------

all:     ../juce_audio_processors.a
posix32: ../juce_audio_processors.posix32.a
posix64: ../juce_audio_processors.posix64.a
win32:   ../juce_audio_processors.win32.a
win64:   ../juce_audio_processors.win64.a

# --------------------------------------------------------------

clean:
	$(RM) *.o ../juce_audio_processors*.a

debug:
	$(MAKE) DEBUG=true

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

# --------------------------------------------------------------

CPP_DEPS = juce_audio_processors.cpp juce_audio_processors.h AppConfig.h */*.cpp */*.h

juce_audio_processors.cpp.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

juce_audio_processors.cpp.%32.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

juce_audio_processors.cpp.%64.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

MM_DEPS = juce_audio_processors.cpp juce_audio_processors.h AppConfig.h */*.mm */*.cpp */*.h

juce_audio_processors.mm.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@

juce_audio_processors.mm.%32.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ $(32BIT_FLAGS) -c -o $@

juce_audio_processors.mm.%64.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
