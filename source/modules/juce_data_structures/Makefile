#!/usr/bin/make -f
# Makefile for juce_data_structures #
# --------------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(JUCE_DATA_STRUCTURES_FLAGS) -I. -w
LINK_FLAGS      += $(JUCE_DATA_STRUCTURES_LIBS) -L.. -ljuce_events -ljuce_core

ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -ObjC++
OBJS         = juce_data_structures.mm.o
OBJS_posix32 = juce_data_structures.mm.posix32.o
OBJS_posix64 = juce_data_structures.mm.posix64.o
else
OBJS         = juce_data_structures.cpp.o
OBJS_posix32 = juce_data_structures.cpp.posix32.o
OBJS_posix64 = juce_data_structures.cpp.posix64.o
endif

OBJS_win32 = juce_data_structures.cpp.win32.o
OBJS_win64 = juce_data_structures.cpp.win64.o

# --------------------------------------------------------------

all: ../juce_data_structures.a

posix32: ../juce_data_structures.posix32.a
posix64: ../juce_data_structures.posix64.a
win32:   ../juce_data_structures.win32.a
win64:   ../juce_data_structures.win64.a

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

../libjuce_data_structures.dll: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

../libjuce_data_structures.dylib: $(OBJS)
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -o $@

../libjuce_data_structures.so: $(OBJS)
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
	$(RM) *.o ../juce_data_structures*.a ../libjuce_data_structures.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
