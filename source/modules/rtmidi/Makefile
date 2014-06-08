#!/usr/bin/make -f
# Makefile for rtmidi #
# ------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(RTMIDI_FLAGS) -I. -Iinclude -I.. -I../../includes

# --------------------------------------------------------------

OBJS = \
	RtMidi.cpp.o

TARGETS = \
	../rtmidi.a

# --------------------------------------------------------------

all: $(TARGETS)

# --------------------------------------------------------------

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

../rtmidi.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

# --------------------------------------------------------------

RtMidi.cpp.o: RtMidi.cpp RtMidi.h RtError.h
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# --------------------------------------------------------------
