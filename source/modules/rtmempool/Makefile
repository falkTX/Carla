#!/usr/bin/make -f
# Makefile for rtmempool #
# ---------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_C_FLAGS += $(RTMEMPOOL_FLAGS) -I. -I../../includes

# --------------------------------------------------------------

OBJS         = rtmempool.c.o
OBJS_posix32 = rtmempool.c.posix32.o
OBJS_posix64 = rtmempool.c.posix64.o
OBJS_win32   = rtmempool.c.win32.o
OBJS_win64   = rtmempool.c.win64.o

# --------------------------------------------------------------

all:     ../rtmempool.a
posix32: ../rtmempool.posix32.a
posix64: ../rtmempool.posix64.a
win32:   ../rtmempool.win32.a
win64:   ../rtmempool.win64.a

# --------------------------------------------------------------

clean:
	$(RM) *.o ../rtmempool*.a

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

../rtmempool.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

../rtmempool.posix32.a: $(OBJS_posix32)
	$(RM) $@
	$(AR) crs $@ $^

../rtmempool.posix64.a: $(OBJS_posix64)
	$(RM) $@
	$(AR) crs $@ $^

../rtmempool.win32.a: $(OBJS_win32)
	$(RM) $@
	$(AR) crs $@ $^

../rtmempool.win64.a: $(OBJS_win64)
	$(RM) $@
	$(AR) crs $@ $^

# --------------------------------------------------------------

DEPS = rtmempool.c rtmempool.h rtmempool-lv2.h list.h

rtmempool.c.o: $(DEPS)
	$(CC) $< $(BUILD_C_FLAGS) -c -o $@

rtmempool.c.%32.o: $(DEPS)
	$(CC) $< $(BUILD_C_FLAGS) $(32BIT_FLAGS) -c -o $@

rtmempool.c.%64.o: $(DEPS)
	$(CC) $< $(BUILD_C_FLAGS) $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
