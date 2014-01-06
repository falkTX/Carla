#!/usr/bin/make -f
# Makefile for lilv #
# ----------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

SERD_VERSION   = 0.18.2
SORD_VERSION   = 0.12.0
SRATOM_VERSION = 0.4.4
LILV_VERSION   = 0.16.0

BUILD_C_FLAGS += $(LILV_FLAGS) -Iconfig -I../../includes -w
LINK_FLAGS    += $(LILV_LIBS)

OBJS         = serd.c.o         sord.c.o         sratom.c.o         lilv.c.o
OBJS_posix32 = serd.c.posix32.o sord.c.posix32.o sratom.c.posix32.o lilv.c.posix32.o
OBJS_posix64 = serd.c.posix64.o sord.c.posix64.o sratom.c.posix64.o lilv.c.posix64.o
OBJS_win32   = serd.c.win32.o   sord.c.win32.o   sratom.c.win32.o   lilv.c.win32.o
OBJS_win64   = serd.c.win64.o   sord.c.win64.o   sratom.c.win64.o   lilv.c.win64.o

# --------------------------------------------------------------

all: ../lilv.a

posix32: ../lilv.posix32.a
posix64: ../lilv.posix64.a
win32:   ../lilv.win32.a
win64:   ../lilv.win64.a

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

../liblilv.dll: $(OBJS)
	$(CC) $^ -shared $(LINK_FLAGS) -o $@

../liblilv.dylib: $(OBJS)
	$(CC) $^ -dynamiclib $(LINK_FLAGS) -o $@

../liblilv.so: $(OBJS)
	$(CC) $^ -shared $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

serd.c.o: serd.c
	$(CC) $< $(BUILD_C_FLAGS) -Iserd-$(SERD_VERSION) -c -o $@

sord.c.o: sord.c
	$(CC) $< $(BUILD_C_FLAGS) -Isord-$(SORD_VERSION) -Isord-$(SORD_VERSION)/src -c -o $@

sratom.c.o: sratom.c
	$(CC) $< $(BUILD_C_FLAGS) -Isratom-$(SRATOM_VERSION) -c -o $@

lilv.c.o: lilv.c
	$(CC) $< $(BUILD_C_FLAGS) -Ililv-$(LILV_VERSION) -Ililv-$(LILV_VERSION)/src -c -o $@

# --------------------------------------------------------------

serd.c.%32.o: serd.c
	$(CC) $< $(BUILD_C_FLAGS) -Iserd-$(SERD_VERSION) $(32BIT_FLAGS) -c -o $@

sord.c.%32.o: sord.c
	$(CC) $< $(BUILD_C_FLAGS) -Isord-$(SORD_VERSION) -Isord-$(SORD_VERSION)/src $(32BIT_FLAGS) -c -o $@

sratom.c.%32.o: sratom.c
	$(CC) $< $(BUILD_C_FLAGS) -Isratom-$(SRATOM_VERSION) $(32BIT_FLAGS) -c -o $@

lilv.c.%32.o: lilv.c
	$(CC) $< $(BUILD_C_FLAGS) -Ililv-$(LILV_VERSION) -Ililv-$(LILV_VERSION)/src $(32BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

serd.c.%64.o: serd.c
	$(CC) $< $(BUILD_C_FLAGS) -Iserd-$(SERD_VERSION) $(64BIT_FLAGS) -c -o $@

sord.c.%64.o: sord.c
	$(CC) $< $(BUILD_C_FLAGS) -Isord-$(SORD_VERSION) -Isord-$(SORD_VERSION)/src $(64BIT_FLAGS) -c -o $@

sratom.c.%64.o: sratom.c
	$(CC) $< $(BUILD_C_FLAGS) -Isratom-$(SRATOM_VERSION) $(64BIT_FLAGS) -c -o $@

lilv.c.%64.o: lilv.c
	$(CC) $< $(BUILD_C_FLAGS) -Ililv-$(LILV_VERSION) -Ililv-$(LILV_VERSION)/src $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

clean:
	$(RM) *.o ../lilv*.a ../liblilv.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
