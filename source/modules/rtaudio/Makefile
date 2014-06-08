#!/usr/bin/make -f
# Makefile for rtaudio #
# -------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(RTAUDIO_FLAGS) -I. -Iinclude -I.. -I../../includes

# --------------------------------------------------------------

OBJS = \
	RtAudio.cpp.o

ifeq ($(WIN32),true)
OBJS += \
	include/asio.cpp.o \
	include/asiodrivers.cpp.o \
	include/asiolist.cpp.o \
	include/iasiothiscallresolver.cpp.o
endif

TARGETS = \
	../rtaudio.a

# --------------------------------------------------------------

all: $(TARGETS)

# --------------------------------------------------------------

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

../rtaudio.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

# --------------------------------------------------------------

RtAudio.cpp.o: RtAudio.cpp RtAudio.h RtError.h
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# --------------------------------------------------------------
