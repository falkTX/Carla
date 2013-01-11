#!/usr/bin/make -f
# Makefile for lilv #
# ----------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

SERD_VERSION   = 0.18.2
SORD_VERSION   = 0.10.4
SRATOM_VERSION = 0.4.0
LILV_VERSION   = 0.14.4

BUILD_C_FLAGS += -fvisibility=hidden -fPIC -w
BUILD_C_FLAGS += -Iconfig -I../../includes

OBJS         = serd.o sord.o sratom.o lilv.o
OBJS_posix32 = serd_posix32.o sord_posix32.o sratom_posix32.o lilv_posix32.o
OBJS_posix64 = serd_posix64.o sord_posix64.o sratom_posix64.o lilv_posix64.o
OBJS_win32   = serd_win32.o sord_win32.o sratom_win32.o lilv_win32.o
OBJS_win64   = serd_win64.o sord_win64.o sratom_win64.o lilv_win64.o

# --------------------------------------------------------------

all: ../lilv.a

posix32: ../lilv_posix32.a
posix64: ../lilv_posix64.a
win32:   ../lilv_win32.a
win64:   ../lilv_win64.a

# --------------------------------------------------------------

../lilv.a: $(OBJS)
	$(AR) rs $@ $^

../lilv_posix32.a: $(OBJS_posix32)
	$(AR) rs $@ $^

../lilv_posix64.a: $(OBJS_posix64)
	$(AR) rs $@ $^

../lilv_win32.a: $(OBJS_win32)
	$(AR) rs $@ $^

../lilv_win64.a: $(OBJS_win64)
	$(AR) rs $@ $^

# --------------------------------------------------------------

serd.o: serd.c
	$(CC) $< $(BUILD_C_FLAGS) -Iserd-$(SERD_VERSION) -c -o $@

sord.o: sord.c
	$(CC) $< $(BUILD_C_FLAGS) -Isord-$(SORD_VERSION) -Isord-$(SORD_VERSION)/src -c -o $@

sratom.o: sratom.c
	$(CC) $< $(BUILD_C_FLAGS) -Isratom-$(SRATOM_VERSION) -c -o $@

lilv.o: lilv.c
	$(CC) $< $(BUILD_C_FLAGS) -Ililv-$(LILV_VERSION) -Ililv-$(LILV_VERSION)/src -c -o $@

# --------------------------------------------------------------

serd_%32.o: serd.c
	$(CC) $< $(BUILD_C_FLAGS) -Iserd-$(SERD_VERSION) $(32BIT_FLAGS) -c -o $@

sord_%32.o: sord.c
	$(CC) $< $(BUILD_C_FLAGS) -Isord-$(SORD_VERSION) -Isord-$(SORD_VERSION)/src $(32BIT_FLAGS) -c -o $@

sratom_%32.o: sratom.c
	$(CC) $< $(BUILD_C_FLAGS) -Isratom-$(SRATOM_VERSION) $(32BIT_FLAGS) -c -o $@

lilv_%32.o: lilv.c
	$(CC) $< $(BUILD_C_FLAGS) -Ililv-$(LILV_VERSION) -Ililv-$(LILV_VERSION)/src $(32BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

serd_%64.o: serd.c
	$(CC) $< $(BUILD_C_FLAGS) -Iserd-$(SERD_VERSION) $(64BIT_FLAGS) -c -o $@

sord_%64.o: sord.c
	$(CC) $< $(BUILD_C_FLAGS) -Isord-$(SORD_VERSION) -Isord-$(SORD_VERSION)/src $(64BIT_FLAGS) -c -o $@

sratom_%64.o: sratom.c
	$(CC) $< $(BUILD_C_FLAGS) -Isratom-$(SRATOM_VERSION) $(64BIT_FLAGS) -c -o $@

lilv_%64.o: lilv.c
	$(CC) $< $(BUILD_C_FLAGS) -Ililv-$(LILV_VERSION) -Ililv-$(LILV_VERSION)/src $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

clean:
	rm -f *.o
