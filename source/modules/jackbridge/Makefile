#!/usr/bin/make -f
# Makefile for jackbridge #
# ------------------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

WINECXX ?= wineg++

BUILD_CXX_FLAGS += -DJACKBRIDGE_EXPORT
BUILD_CXX_FLAGS += -I. -I../../includes -I../../utils

ifeq ($(MACOS),true)
LINK_FLAGS += -dynamiclib
else
LINK_FLAGS += -shared
endif

WIN_BUILD_FLAGS  = $(BUILD_CXX_FLAGS) -DJACKBRIDGE_DUMMY=1 -w
WIN_32BIT_FLAGS  = $(32BIT_FLAGS)
WIN_64BIT_FLAGS  = $(64BIT_FLAGS)
WIN_LINK_FLAGS   = $(LINK_FLAGS)

WINE_BUILD_FLAGS = $(BUILD_CXX_FLAGS) -fPIC
WINE_32BIT_FLAGS = $(32BIT_FLAGS) -L/usr/lib32/wine -L/usr/lib/i386-linux-gnu/wine
WINE_64BIT_FLAGS = $(64BIT_FLAGS) -L/usr/lib64/wine -L/usr/lib/x86_64-linux-gnu/wine
WINE_LINK_FLAGS  = $(LINK_FLAGS) -ldl -lrt -lpthread

OBJS    = JackBridge1.cpp   JackBridge2.cpp
OBJSw32 = JackBridge1.w32.o JackBridge2.w32.o
OBJSw64 = JackBridge1.w64.o JackBridge2.w64.o

# --------------------------------------------------------------

all:

win32:  ../jackbridge-win32.dll
win64:  ../jackbridge-win64.dll
wine32: ../jackbridge-win32.dll.so
wine64: ../jackbridge-win64.dll.so

# --------------------------------------------------------------

JackBridge%.w32.o: JackBridge%.cpp
	$(CXX) $^ $(WINE_BUILD_FLAGS) $(WINE_32BIT_FLAGS) -c -o $@

JackBridge%.w64.o: JackBridge%.cpp
	$(CXX) $^ $(WINE_BUILD_FLAGS) $(WINE_64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

../jackbridge-win32.dll: $(OBJS)
	$(CXX) $^ $(WIN_BUILD_FLAGS) $(WIN_32BIT_FLAGS) $(WIN_LINK_FLAGS) -Wl,--output-def,$@.def,--out-implib,$@.a -o $@ $(CMD_STRIP) $@

../jackbridge-win64.dll: $(OBJS)
	$(CXX) $^ $(WIN_BUILD_FLAGS) $(WIN_64BIT_FLAGS) $(WIN_LINK_FLAGS) -Wl,--output-def,$@.def,--out-implib,$@.a -o $@ $(CMD_STRIP) $@

../jackbridge-win32.dll.so: $(OBJSw32) ../jackbridge-win32.dll.def
	$(WINECXX) $^ $(WINE_32BIT_FLAGS) $(WINE_LINK_FLAGS) -o $@ $(CMD_STRIP) $@

../jackbridge-win64.dll.so: $(OBJS) ../jackbridge-win64.dll.def
	$(WINECXX) $^ $(WINE_64BIT_FLAGS) $(WINE_LINK_FLAGS) -o $@ $(CMD_STRIP) $@

# --------------------------------------------------------------
