#!/usr/bin/make -f
# Makefile for carla interposer #
# ----------------------------- #
# Created by falkTX
#

CWD=..
include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I$(CWD)/includes -I$(CWD)/modules -I$(CWD)/utils

# --------------------------------------------------------------

ifeq ($(LINUX),true)
TARGETS = ../../bin/libcarla_interposer.so
endif

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

../../bin/libcarla_interposer.so: interposer.cpp.o
	$(CXX) $< $(SHARED) $(LINK_FLAGS) -ldl -o $@

# --------------------------------------------------------------
