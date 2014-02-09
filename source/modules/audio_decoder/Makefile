#!/usr/bin/make -f
# Makefile for audio_decoder #
# -------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_C_FLAGS += $(AUDIO_DECODER_FLAGS)
LINK_FLAGS    += $(AUDIO_DECODER_LIBS)

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

serd.c.o: serd.c
	$(CC) $< $(BUILD_C_FLAGS) -Iserd-$(SERD_VERSION) -c -o $@

sord.c.o: sord.c
	$(CC) $< $(BUILD_C_FLAGS) -Isord-$(SORD_VERSION) -Isord-$(SORD_VERSION)/src -c -o $@

sratom.c.o: sratom.c
	$(CC) $< $(BUILD_C_FLAGS) -Isratom-$(SRATOM_VERSION) -c -o $@

audio_decoder.c.o: audio_decoder.c
	$(CC) $< $(BUILD_C_FLAGS) -Iaudio_decoder-$(LILV_VERSION) -Iaudio_decoder-$(LILV_VERSION)/src -c -o $@

# --------------------------------------------------------------

serd.c.%32.o: serd.c
	$(CC) $< $(BUILD_C_FLAGS) -Iserd-$(SERD_VERSION) $(32BIT_FLAGS) -c -o $@

sord.c.%32.o: sord.c
	$(CC) $< $(BUILD_C_FLAGS) -Isord-$(SORD_VERSION) -Isord-$(SORD_VERSION)/src $(32BIT_FLAGS) -c -o $@

sratom.c.%32.o: sratom.c
	$(CC) $< $(BUILD_C_FLAGS) -Isratom-$(SRATOM_VERSION) $(32BIT_FLAGS) -c -o $@

audio_decoder.c.%32.o: audio_decoder.c
	$(CC) $< $(BUILD_C_FLAGS) -Iaudio_decoder-$(LILV_VERSION) -Iaudio_decoder-$(LILV_VERSION)/src $(32BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

serd.c.%64.o: serd.c
	$(CC) $< $(BUILD_C_FLAGS) -Iserd-$(SERD_VERSION) $(64BIT_FLAGS) -c -o $@

sord.c.%64.o: sord.c
	$(CC) $< $(BUILD_C_FLAGS) -Isord-$(SORD_VERSION) -Isord-$(SORD_VERSION)/src $(64BIT_FLAGS) -c -o $@

sratom.c.%64.o: sratom.c
	$(CC) $< $(BUILD_C_FLAGS) -Isratom-$(SRATOM_VERSION) $(64BIT_FLAGS) -c -o $@

audio_decoder.c.%64.o: audio_decoder.c
	$(CC) $< $(BUILD_C_FLAGS) -Iaudio_decoder-$(LILV_VERSION) -Iaudio_decoder-$(LILV_VERSION)/src $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

clean:
	$(RM) *.o ../audio_decoder*.a ../libaudio_decoder.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
