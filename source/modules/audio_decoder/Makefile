#!/usr/bin/make -f
# Makefile for audio_decoder #
# -------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_C_FLAGS += $(AUDIO_DECODER_FLAGS) -I..
LINK_FLAGS    += $(AUDIO_DECODER_LIBS)

OBJS         = ad_ffmpeg.c.o         ad_plugin.c.o         ad_soundfile.c.o
OBJS_posix32 = ad_ffmpeg.c.posix32.o ad_plugin.c.posix32.o ad_soundfile.c.posix32.o
OBJS_posix64 = ad_ffmpeg.c.posix64.o ad_plugin.c.posix64.o ad_soundfile.c.posix64.o
OBJS_win32   = ad_ffmpeg.c.win32.o   ad_plugin.c.win32.o   ad_soundfile.c.win32.o
OBJS_win64   = ad_ffmpeg.c.win64.o   ad_plugin.c.win64.o   ad_soundfile.c.win64.o

# --------------------------------------------------------------

all: ../audio_decoder.a

posix32: ../audio_decoder.posix32.a
posix64: ../audio_decoder.posix64.a
win32:   ../audio_decoder.win32.a
win64:   ../audio_decoder.win64.a

# --------------------------------------------------------------

../audio_decoder.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

../audio_decoder.posix32.a: $(OBJS_posix32)
	$(RM) $@
	$(AR) crs $@ $^

../audio_decoder.posix64.a: $(OBJS_posix64)
	$(RM) $@
	$(AR) crs $@ $^

../audio_decoder.win32.a: $(OBJS_win32)
	$(RM) $@
	$(AR) crs $@ $^

../audio_decoder.win64.a: $(OBJS_win64)
	$(RM) $@
	$(AR) crs $@ $^

../libaudio_decoder.dll: $(OBJS)
	$(CC) $^ -shared $(LINK_FLAGS) -o $@

../libaudio_decoder.dylib: $(OBJS)
	$(CC) $^ -dynamiclib $(LINK_FLAGS) -o $@

../libaudio_decoder.so: $(OBJS)
	$(CC) $^ -shared $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

%.c.o: %.c
	$(CC) $< $(BUILD_C_FLAGS) -c -o $@

%.c.posix32.o: %.c
	$(CC) $< $(BUILD_C_FLAGS) $(32BIT_FLAGS) -c -o $@

%.c.posix64.o: %.c
	$(CC) $< $(BUILD_C_FLAGS) $(64BIT_FLAGS) -c -o $@

%.c.win32.o: %.c
	$(CC) $< $(BUILD_C_FLAGS) $(32BIT_FLAGS) -c -o $@

%.c.win64.o: %.c
	$(CC) $< $(BUILD_C_FLAGS) $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

clean:
	$(RM) *.o ../audio_decoder*.a ../libaudio_decoder.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
