#!/usr/bin/make -f
# Makefile for juce_gui_extra #
# --------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(JUCE_GUI_EXTRA_FLAGS) -I. -w

# --------------------------------------------------------------

ifeq ($(MACOS),true)
OBJS         = juce_gui_extra.mm.o
OBJS_posix32 = juce_gui_extra.mm.posix32.o
OBJS_posix64 = juce_gui_extra.mm.posix64.o
else
OBJS         = juce_gui_extra.cpp.o
OBJS_posix32 = juce_gui_extra.cpp.posix32.o
OBJS_posix64 = juce_gui_extra.cpp.posix64.o
endif
OBJS_win32   = juce_gui_extra.cpp.win32.o
OBJS_win64   = juce_gui_extra.cpp.win64.o

# --------------------------------------------------------------

all:     ../juce_gui_extra.a
posix32: ../juce_gui_extra.posix32.a
posix64: ../juce_gui_extra.posix64.a
win32:   ../juce_gui_extra.win32.a
win64:   ../juce_gui_extra.win64.a

# --------------------------------------------------------------

clean:
	$(RM) *.o ../juce_gui_extra*.a

debug:
	$(MAKE) DEBUG=true

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

# --------------------------------------------------------------

CPP_DEPS = juce_gui_extra.cpp juce_gui_extra.h AppConfig.h */*.cpp */*.h

juce_gui_extra.cpp.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

juce_gui_extra.cpp.%32.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

juce_gui_extra.cpp.%64.o: $(CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

MM_DEPS = juce_gui_extra.mm juce_gui_extra.h AppConfig.h */*.mm */*.cpp */*.h

juce_gui_extra.mm.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@

juce_gui_extra.mm.%32.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ $(32BIT_FLAGS) -c -o $@

juce_gui_extra.mm.%64.o: $(MM_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
