#!/usr/bin/make -f
# Makefile for audio_decoder #
# -------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_C_FLAGS += $(AUDIO_DECODER_FLAGS) -I. -I..

# --------------------------------------------------------------

OBJS = \
	ad_ffmpeg.c.o \
	ad_plugin.c.o \
	ad_soundfile.c.o

TARGETS = \
	../audio_decoder.a

# --------------------------------------------------------------

all: $(TARGETS)

# --------------------------------------------------------------

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

../audio_decoder.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

# --------------------------------------------------------------

ad_ffmpeg.c.o: ad_ffmpeg.c ad_plugin.h ffcompat.h ad.h
	$(CC) $< $(BUILD_C_FLAGS) -c -o $@

ad_plugin.c.o: ad_plugin.c ad_plugin.h ad.h
	$(CC) $< $(BUILD_C_FLAGS) -c -o $@

ad_soundfile.c.o: ad_soundfile.c ad_plugin.h ad.h
	$(CC) $< $(BUILD_C_FLAGS) -c -o $@

# --------------------------------------------------------------
