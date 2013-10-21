#!/usr/bin/make -f
# Makefile for jackbridge #
# ----------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

WINECXX ?= wineg++

BUILD_CXX_FLAGS += $(JACKBRIDGE_FLAGS) -I. -I../../includes -I../../utils
LINK_FLAGS      += $(JACKBRIDGE_LIBS)
# linux: -ldl -lpthread -lrt
# mac: -ldl -lpthread
# win: -lpthread

OBJS         = JackBridge1.cpp.o         JackBridge2.cpp.o
OBJS_posix32 = JackBridge1.cpp.posix32.o JackBridge2.cpp.posix32.o
OBJS_posix64 = JackBridge1.cpp.posix64.o JackBridge2.cpp.posix64.o
OBJS_win32   = JackBridge1.cpp.win32.o   JackBridge2.cpp.win32.o
OBJS_win64   = JackBridge1.cpp.win64.o   JackBridge2.cpp.win64.o

# --------------------------------------------------------------

all: ../jackbridge.a

posix32: ../jackbridge.posix32.a
posix64: ../jackbridge.posix64.a
win32:   ../jackbridge.win32.a
win64:   ../jackbridge.win64.a

# --------------------------------------------------------------

../jackbridge.a: $(OBJS)
	rm -f $@
	$(AR) crs $@ $^

../jackbridge.posix32.a: $(OBJS_posix32)
	rm -f $@
	$(AR) crs $@ $^

../jackbridge.posix64.a: $(OBJS_posix64)
	rm -f $@
	$(AR) crs $@ $^

../jackbridge.win32.a: $(OBJS_win32)
	rm -f $@
	$(AR) crs $@ $^

../jackbridge.win64.a: $(OBJS_win64)
	rm -f $@
	$(AR) crs $@ $^

../libjackbridge.dll: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

../libjackbridge.dylib: $(OBJS)
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -o $@

../libjackbridge.so: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

%.cpp.o: %.cpp
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
	rm -f *.o ../jackbridge*.a ../libjackbridge.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

# win32:  ../jackbridge-win32.dll
# win64:  ../jackbridge-win64.dll
# wine32: ../jackbridge-win32.dll.so
# wine64: ../jackbridge-win64.dll.so

# BUILD_CXX_FLAGS += -DJACKBRIDGE_EXPORT

# WIN_BUILD_FLAGS  = $(BUILD_CXX_FLAGS) -DJACKBRIDGE_DUMMY=1
# WIN_32BIT_FLAGS  = $(32BIT_FLAGS)
# WIN_64BIT_FLAGS  = $(64BIT_FLAGS)
# WIN_LINK_FLAGS   = $(LINK_FLAGS)
#
# WINE_BUILD_FLAGS = $(BUILD_CXX_FLAGS) -fPIC
# WINE_32BIT_FLAGS = $(32BIT_FLAGS) -L/usr/lib32/wine -L/usr/lib/i386-linux-gnu/wine
# WINE_64BIT_FLAGS = $(64BIT_FLAGS) -L/usr/lib64/wine -L/usr/lib/x86_64-linux-gnu/wine
# WINE_LINK_FLAGS  = $(LINK_FLAGS) -ldl -lrt -lpthread

# OBJSw32 = JackBridge1.w32.o JackBridge2.w32.o
# OBJSw64 = JackBridge1.w64.o JackBridge2.w64.o

# JackBridge%.w32.o: JackBridge%.cpp
# 	$(CXX) $^ $(WINE_BUILD_FLAGS) $(WINE_32BIT_FLAGS) -c -o $@
#
# JackBridge%.w64.o: JackBridge%.cpp
# 	$(CXX) $^ $(WINE_BUILD_FLAGS) $(WINE_64BIT_FLAGS) -c -o $@

# ../jackbridge-win32.dll: $(OBJS)
# 	$(CXX) $^ $(WIN_BUILD_FLAGS) $(WIN_32BIT_FLAGS) $(WIN_LINK_FLAGS) -Wl,--output-def,$@.def,--out-implib,$@.a -o $@
#
# ../jackbridge-win64.dll: $(OBJS)
# 	$(CXX) $^ $(WIN_BUILD_FLAGS) $(WIN_64BIT_FLAGS) $(WIN_LINK_FLAGS) -Wl,--output-def,$@.def,--out-implib,$@.a -o $@
#
# ../jackbridge-win32.dll.so: $(OBJSw32) ../jackbridge-win32.dll.def
# 	$(WINECXX) $^ $(WINE_32BIT_FLAGS) $(WINE_LINK_FLAGS) -o $@
#
# ../jackbridge-win64.dll.so: $(OBJS) ../jackbridge-win64.dll.def
# 	$(WINECXX) $^ $(WINE_64BIT_FLAGS) $(WINE_LINK_FLAGS) -o $@
