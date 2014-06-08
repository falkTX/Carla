#!/usr/bin/make -f
# Makefile for juce_gui_extra #
# --------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(JUCE_GUI_EXTRA_FLAGS) -I. -w
LINK_FLAGS      += $(JUCE_GUI_EXTRA_LIBS) -L.. -ljuce_gui_basics -ljuce_graphics -ljuce_data_structures -ljuce_events -ljuce_core

ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -ObjC++
OBJS         = juce_gui_extra.mm.o
OBJS_posix32 = juce_gui_extra.mm.posix32.o
OBJS_posix64 = juce_gui_extra.mm.posix64.o
else
OBJS         = juce_gui_extra.cpp.o
OBJS_posix32 = juce_gui_extra.cpp.posix32.o
OBJS_posix64 = juce_gui_extra.cpp.posix64.o
endif

OBJS_win32 = juce_gui_extra.cpp.win32.o
OBJS_win64 = juce_gui_extra.cpp.win64.o

# --------------------------------------------------------------

all: ../juce_gui_extra.a

posix32: ../juce_gui_extra.posix32.a
posix64: ../juce_gui_extra.posix64.a
win32:   ../juce_gui_extra.win32.a
win64:   ../juce_gui_extra.win64.a

# --------------------------------------------------------------

../juce_gui_extra.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

../juce_gui_extra.posix32.a: $(OBJS_posix32)
	$(RM) $@
	$(AR) crs $@ $^

../juce_gui_extra.posix64.a: $(OBJS_posix64)
	$(RM) $@
	$(AR) crs $@ $^

../juce_gui_extra.win32.a: $(OBJS_win32)
	$(RM) $@
	$(AR) crs $@ $^

../juce_gui_extra.win64.a: $(OBJS_win64)
	$(RM) $@
	$(AR) crs $@ $^

../libjuce_gui_extra.dll: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

../libjuce_gui_extra.dylib: $(OBJS)
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -o $@

../libjuce_gui_extra.so: $(OBJS)
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
	$(RM) *.o ../juce_gui_extra*.a ../libjuce_gui_extra.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
