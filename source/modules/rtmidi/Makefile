#!/usr/bin/make -f
# Makefile for rtmidi #
# ------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(RTMIDI_FLAGS) -I. -Iinclude -I.. -I../../includes -w
LINK_FLAGS      += $(RTMIDI_LIBS)

OBJS = RtMidi.cpp.o

# --------------------------------------------------------------

all: ../rtmidi.a

# --------------------------------------------------------------

../rtmidi.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

../librtmidi.dll: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

../librtmidi.dylib: $(OBJS)
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -o $@

../librtmidi.so: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

%.cpp.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# --------------------------------------------------------------

clean:
	$(RM) *.o ../rtmidi*.a ../librtmidi.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
