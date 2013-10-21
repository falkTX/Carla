#!/usr/bin/make -f
# Makefile for rtaudio #
# -------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(RTAUDIO_FLAGS) -DHAVE_GETTIMEOFDAY -D__UNIX_JACK__ -I. -Iinclude -I.. -I../../includes
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

ifeq ($(DEBUG),true)
BUILD_CXX_FLAGS += -D__RTAUDIO_DEBUG__
endif

ifeq ($(HAVE_ALSA),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags alsa) -D__LINUX_ALSA__
endif

ifeq ($(HAVE_PULSEAUDIO),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags libpulse-simple) -D__LINUX_PULSE__
endif

ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -D__MACOSX_CORE__
endif

ifeq ($(WIN32),true)
BUILD_CXX_FLAGS += -D__WINDOWS_ASIO__ -D__WINDOWS_DS__
endif

# --------------------------------------------------------------

all: ../rtaudio.a

posix32: ../rtaudio.posix32.a
posix64: ../rtaudio.posix64.a
win32:   ../rtaudio.win32.a
win64:   ../rtaudio.win64.a

# --------------------------------------------------------------

../rtaudio.a: $(OBJS)
	rm -f $@
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
	rm -f *.o ../rtaudio*.a ../librtaudio.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
