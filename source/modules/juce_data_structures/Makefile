#!/usr/bin/make -f
# Makefile for juce_data_structures #
# --------------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(JUCE_DATA_STRUCTURES_FLAGS) -I. -w

# --------------------------------------------------------------

ifeq ($(MACOS),true)
OBJS         = juce_data_structures.mm.o
OBJS_posix32 = juce_data_structures.mm.posix32.o
OBJS_posix64 = juce_data_structures.mm.posix64.o
else
OBJS         = juce_data_structures.cpp.o
OBJS_posix32 = juce_data_structures.cpp.posix32.o
OBJS_posix64 = juce_data_structures.cpp.posix64.o
endif
OBJS_win32   = juce_data_structures.cpp.win32.o
OBJS_win64   = juce_data_structures.cpp.win64.o

# --------------------------------------------------------------

all:     ../juce_data_structures.a
posix32: ../juce_data_structures.posix32.a
posix64: ../juce_data_structures.posix64.a
win32:   ../juce_data_structures.win32.a
win64:   ../juce_data_structures.win64.a

# --------------------------------------------------------------

clean:
	$(RM) *.o ../juce_data_structures*.a

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

../juce_data_structures.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

../juce_data_structures.posix32.a: $(OBJS_posix32)
	$(RM) $@
	$(AR) crs $@ $^

../juce_data_structures.posix64.a: $(OBJS_posix64)
	$(RM) $@
	$(AR) crs $@ $^

../juce_data_structures.win32.a: $(OBJS_win32)
	$(RM) $@
	$(AR) crs $@ $^

../juce_data_structures.win64.a: $(OBJS_win64)
	$(RM) $@
	$(AR) crs $@ $^

# --------------------------------------------------------------

CPP_DEPS = juce_data_structures.cpp juce_data_structures.h AppConfig.h */*.cpp */*.h

juce_data_structures.cpp.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

juce_data_structures.cpp.%32.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

juce_data_structures.cpp.%64.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

MM_DEPS = juce_data_structures.mm juce_data_structures.h AppConfig.h */*.cpp */*.h

juce_data_structures.mm.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@

juce_data_structures.mm.%32.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ $(32BIT_FLAGS) -c -o $@

juce_data_structures.mm.%64.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
