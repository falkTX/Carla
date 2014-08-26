#!/usr/bin/make -f
# Makefile for lilv #
# ----------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

SERD_VERSION   = 0.20.0
SORD_VERSION   = 0.12.2
SRATOM_VERSION = 0.4.6
LILV_VERSION   = 0.20.0

# --------------------------------------------------------------

BUILD_C_FLAGS += $(LILV_FLAGS) -Iconfig -I../../includes -w
LINK_FLAGS    += $(LILV_LIBS)

# --------------------------------------------------------------

OBJS         = serd.c.o         sord.c.o         sratom.c.o         lilv.c.o
OBJS_posix32 = serd.c.posix32.o sord.c.posix32.o sratom.c.posix32.o lilv.c.posix32.o
OBJS_posix64 = serd.c.posix64.o sord.c.posix64.o sratom.c.posix64.o lilv.c.posix64.o
OBJS_win32   = serd.c.win32.o   sord.c.win32.o   sratom.c.win32.o   lilv.c.win32.o
OBJS_win64   = serd.c.win64.o   sord.c.win64.o   sratom.c.win64.o   lilv.c.win64.o

# --------------------------------------------------------------

all:     ../lilv.a
posix32: ../lilv.posix32.a
posix64: ../lilv.posix64.a
win32:   ../lilv.win32.a
win64:   ../lilv.win64.a

# --------------------------------------------------------------

clean:
	$(RM) *.o ../lilv*.a

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

../lilv.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

../lilv.posix32.a: $(OBJS_posix32)
	$(RM) $@
	$(AR) crs $@ $^

../lilv.posix64.a: $(OBJS_posix64)
	$(RM) $@
	$(AR) crs $@ $^

../lilv.win32.a: $(OBJS_win32)
	$(RM) $@
	$(AR) crs $@ $^

../lilv.win64.a: $(OBJS_win64)
	$(RM) $@
	$(AR) crs $@ $^

# --------------------------------------------------------------

SERD_DEPS = serd.c serd-$(SERD_VERSION)/src/* serd-$(SERD_VERSION)/serd/serd.h

serd.c.o: $(SERD_DEPS)
	$(CC) $< $(BUILD_C_FLAGS) -Iserd-$(SERD_VERSION) -c -o $@

serd.c.%32.o: $(SERD_DEPS)
	$(CC) $< $(BUILD_C_FLAGS) -Iserd-$(SERD_VERSION) $(32BIT_FLAGS) -c -o $@

serd.c.%64.o: $(SERD_DEPS)
	$(CC) $< $(BUILD_C_FLAGS) -Iserd-$(SERD_VERSION) $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

SORD_DEPS = sord.c sord-$(SORD_VERSION)/src/* sord-$(SORD_VERSION)/sord/sord.h

sord.c.o: $(SORD_DEPS)
	$(CC) $< $(BUILD_C_FLAGS) -Isord-$(SORD_VERSION) -Isord-$(SORD_VERSION)/src -c -o $@

sord.c.%32.o: $(SORD_DEPS)
	$(CC) $< $(BUILD_C_FLAGS) -Isord-$(SORD_VERSION) -Isord-$(SORD_VERSION)/src $(32BIT_FLAGS) -c -o $@

sord.c.%64.o: $(SORD_DEPS)
	$(CC) $< $(BUILD_C_FLAGS) -Isord-$(SORD_VERSION) -Isord-$(SORD_VERSION)/src $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

SRATOM_DEPS = sratom.c sratom-$(SRATOM_VERSION)/src/* sratom-$(SRATOM_VERSION)/sratom/sratom.h

sratom.c.o: $(SRATOM_DEPS)
	$(CC) $< $(BUILD_C_FLAGS) -Isratom-$(SRATOM_VERSION) -c -o $@

sratom.c.%32.o: $(SRATOM_DEPS)
	$(CC) $< $(BUILD_C_FLAGS) -Isratom-$(SRATOM_VERSION) $(32BIT_FLAGS) -c -o $@

sratom.c.%64.o: $(SRATOM_DEPS)
	$(CC) $< $(BUILD_C_FLAGS) -Isratom-$(SRATOM_VERSION) $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

LILV_DEPS = lilv.c lilv-$(LILV_VERSION)/src/* lilv-$(LILV_VERSION)/src/zix/* lilv-$(LILV_VERSION)/lilv/lilv.h

lilv.c.o: $(LILV_DEPS)
	$(CC) $< $(BUILD_C_FLAGS) -Ililv-$(LILV_VERSION) -Ililv-$(LILV_VERSION)/src -c -o $@

lilv.c.%32.o: $(LILV_DEPS)
	$(CC) $< $(BUILD_C_FLAGS) -Ililv-$(LILV_VERSION) -Ililv-$(LILV_VERSION)/src $(32BIT_FLAGS) -c -o $@

lilv.c.%64.o: $(LILV_DEPS)
	$(CC) $< $(BUILD_C_FLAGS) -Ililv-$(LILV_VERSION) -Ililv-$(LILV_VERSION)/src $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
