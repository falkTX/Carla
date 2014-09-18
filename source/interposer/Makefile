#!/usr/bin/make -f
# Makefile for carla interposer #
# ----------------------------- #
# Created by falkTX
#

CWD=..
include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../includes -I../utils -isystem ../modules

# --------------------------------------------------------------

TARGETS = ../../bin/libcarlainterposer$(LIB_EXT)

# --------------------------------------------------------------

all: $(TARGETS)

clean:
	$(RM) $(TARGETS) *.o

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

interposer.cpp.o: interposer.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# --------------------------------------------------------------

../../bin/libcarlainterposer$(LIB_EXT): interposer.cpp.o
	$(CXX) $< $(SHARED) $(LINK_FLAGS) -ldl -o $@

# --------------------------------------------------------------
