#!/usr/bin/make -f
# Makefile for rtaudio #
# -------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(RTAUDIO_FLAGS) -I. -Iinclude -I.. -I../../includes -w
LINK_FLAGS      += $(RTAUDIO_LIBS) -L.. -ljackbridge

OBJS = RtAudio.cpp.o

ifeq ($(WIN32),true)
OBJS += \
	include/asio.cpp.o \
	include/asiodrivers.cpp.o \
	include/asiolist.cpp.o \
	include/iasiothiscallresolver.cpp.o
endif

# --------------------------------------------------------------

all: ../rtaudio.a

# --------------------------------------------------------------

../rtaudio.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

../librtaudio.dll: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

../librtaudio.dylib: $(OBJS)
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -o $@

../librtaudio.so: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

%.cpp.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# --------------------------------------------------------------

clean:
	$(RM) *.o ../rtaudio*.a ../librtaudio.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
