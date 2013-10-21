#!/usr/bin/make -f
# Makefile for stk #
# ---------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(STK_FLAGS) -I.
LINK_FLAGS      += $(STK_LIBS)

ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -objc++
OBJS         = stk.mm.o
OBJS_posix32 = stk.mm.posix32.o
OBJS_posix64 = stk.mm.posix64.o
else
OBJS         = stk.cpp.o
OBJS_posix32 = stk.cpp.posix32.o
OBJS_posix64 = stk.cpp.posix64.o
endif

OBJS_win32 = stk.cpp.win32.o
OBJS_win64 = stk.cpp.win64.o

# --------------------------------------------------------------

all: ../stk.a

posix32: ../stk.posix32.a
posix64: ../stk.posix64.a
win32:   ../stk.win32.a
win64:   ../stk.win64.a

# --------------------------------------------------------------

../stk.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

../stk.posix32.a: $(OBJS_posix32)
	$(RM) $@
	$(AR) crs $@ $^

../stk.posix64.a: $(OBJS_posix64)
	$(RM) $@
	$(AR) crs $@ $^

../stk.win32.a: $(OBJS_win32)
	$(RM) $@
	$(AR) crs $@ $^

../stk.win64.a: $(OBJS_win64)
	$(RM) $@
	$(AR) crs $@ $^

../libstk.dll: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

../libstk.dylib: $(OBJS)
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -o $@

../libstk.so: $(OBJS)
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
	$(RM) *.o ../stk*.a ../libstk.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
