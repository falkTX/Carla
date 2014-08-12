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

WINE_32BIT_FLAGS = $(32BIT_FLAGS) -L/usr/lib32/wine -L/usr/lib/i386-linux-gnu/wine
WINE_64BIT_FLAGS = $(64BIT_FLAGS) -L/usr/lib64/wine -L/usr/lib/x86_64-linux-gnu/wine
WINE_LINK_FLAGS  = $(LINK_FLAGS) -ldl -lrt -lpthread

# --------------------------------------------------------------

OBJS          = JackBridge1.cpp.o          JackBridge2.cpp.o
OBJS_posix32  = JackBridge1.cpp.posix32.o  JackBridge2.cpp.posix32.o
OBJS_posix64  = JackBridge1.cpp.posix64.o  JackBridge2.cpp.posix64.o
OBJS_win32    = JackBridge1.cpp.win32.o    JackBridge2.cpp.win32.o
OBJS_win64    = JackBridge1.cpp.win64.o    JackBridge2.cpp.win64.o

OBJS_posix32e = JackBridgeExport.cpp.posix32e.o
OBJS_posix64e = JackBridgeExport.cpp.posix64e.o
OBJS_win64e   = JackBridgeExport.cpp.win64e.o
OBJS_win32e   = JackBridgeExport.cpp.win32e.o

OBJS_wine32   = JackBridge1.cpp.wine32.o JackBridge2.cpp.wine32.o JackBridge3.cpp.wine32.o
OBJS_wine64   = JackBridge1.cpp.wine64.o JackBridge2.cpp.wine64.o JackBridge3.cpp.wine64.o

# --------------------------------------------------------------

all: ../jackbridge.a

posix32:  ../jackbridge.posix32.a
posix64:  ../jackbridge.posix64.a
posix32e: ../jackbridge.posix32e.a
posix64e: ../jackbridge.posix64e.a

win32:   ../jackbridge.win32.a
win64:   ../jackbridge.win64.a
win32e:  ../jackbridge.win32e.a
win64e:  ../jackbridge.win64e.a

wine32:  ../jackbridge-wine32.dll.so
wine64:  ../jackbridge-wine64.dll.so

# --------------------------------------------------------------

clean:
	$(RM) *.o ../jackbridge*.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

../jackbridge.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

../jackbridge.posix32.a: $(OBJS_posix32)
	$(RM) $@
	$(AR) crs $@ $^

../jackbridge.posix64.a: $(OBJS_posix64)
	$(RM) $@
	$(AR) crs $@ $^

../jackbridge.win32.a: $(OBJS_win32)
	$(RM) $@
	$(AR) crs $@ $^

../jackbridge.win64.a: $(OBJS_win64)
	$(RM) $@
	$(AR) crs $@ $^

# --------------------------------------------------------------

../jackbridge.posix32e.a: $(OBJS_posix32e)
	$(RM) $@
	$(AR) crs $@ $^

../jackbridge.posix64e.a: $(OBJS_posix64e)
	$(RM) $@
	$(AR) crs $@ $^

../jackbridge.win32e.a: $(OBJS_win32e)
	$(RM) $@
	$(AR) crs $@ $^

../jackbridge.win64e.a: $(OBJS_win64e)
	$(RM) $@
	$(AR) crs $@ $^

# --------------------------------------------------------------

../jackbridge-wine32.dll.so: $(OBJS_wine32) JackBridgeExport.def
	$(WINECXX) $^ -shared $(WINE_32BIT_FLAGS) $(WINE_LINK_FLAGS) -o $@

../jackbridge-wine64.dll.so: $(OBJS_wine64) JackBridgeExport.def
	$(WINECXX) $^ -shared $(WINE_64BIT_FLAGS) $(WINE_LINK_FLAGS) -o $@

# --------------------------------------------------------------

JackBridge1.cpp.o: JackBridge1.cpp JackBridge.hpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

JackBridge2.cpp.o: JackBridge2.cpp JackBridge.hpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

JackBridgeExport.cpp.%32e.o: JackBridgeExport.cpp JackBridgeExport.hpp JackBridge.hpp
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -fpermissive -c -o $@

JackBridgeExport.cpp.%64e.o: JackBridgeExport.cpp JackBridgeExport.hpp JackBridge.hpp
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -fpermissive -c -o $@

# --------------------------------------------------------------

%.posix32.o: %
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

%.posix64.o: %
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

%.win32.o: %
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

%.win64.o: %
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

%.wine32.o: %
	$(WINECXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

%.wine64.o: %
	$(WINECXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
