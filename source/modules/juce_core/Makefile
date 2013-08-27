#!/usr/bin/make -f
# Makefile for juce_core #
# ---------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I.

ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -objc++
OBJS         = juce_core.mm.o
OBJS_posix32 = juce_core.mm.posix32.o
OBJS_posix64 = juce_core.mm.posix64.o
else
OBJS         = juce_core.cpp.o
OBJS_posix32 = juce_core.cpp.posix32.o
OBJS_posix64 = juce_core.cpp.posix64.o
endif

OBJS_win32 = juce_core.cpp.win32.o
OBJS_win64 = juce_core.cpp.win64.o

# --------------------------------------------------------------

all: ../juce_core.a

posix32: ../juce_core.posix32.a
posix64: ../juce_core.posix64.a
win32:   ../juce_core.win32.a
win64:   ../juce_core.win64.a

# --------------------------------------------------------------

../juce_core.a: $(OBJS)
	$(AR) rs $@ $^

../juce_core.posix32.a: $(OBJS_posix32)
	$(AR) rs $@ $^

../juce_core.posix64.a: $(OBJS_posix64)
	$(AR) rs $@ $^

../juce_core.win32.a: $(OBJS_win32)
	$(AR) rs $@ $^

../juce_core.win64.a: $(OBJS_win64)
	$(AR) rs $@ $^

../juce_core.dll: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

../juce_core.dylib: $(OBJS)
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -o $@

../juce_core.so: $(OBJS)
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
	rm -f *.o ../juce_core*.a

debug:
	$(MAKE) DEBUG=true
