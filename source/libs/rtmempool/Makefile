#!/usr/bin/make -f
# Makefile for rtmempool #
# ----------------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_C_FLAGS += -fvisibility=hidden -fPIC
BUILD_C_FLAGS += -I. -I../../includes

OBJS         = rtmempool.o
OBJS_posix32 = rtmempool_posix32.o
OBJS_posix64 = rtmempool_posix64.o
OBJS_win32   = rtmempool_win32.o
OBJS_win64   = rtmempool_win64.o

# --------------------------------------------------------------

all: ../rtmempool.a

posix32: ../rtmempool_posix32.a
posix64: ../rtmempool_posix64.a
win32:   ../rtmempool_win32.a
win64:   ../rtmempool_win64.a

# --------------------------------------------------------------

../rtmempool.a: $(OBJS)
	$(AR) rs $@ $^

../rtmempool_posix32.a: $(OBJS_posix32)
	$(AR) rs $@ $^

../rtmempool_posix64.a: $(OBJS_posix64)
	$(AR) rs $@ $^

../rtmempool_win32.a: $(OBJS_win32)
	$(AR) rs $@ $^

../rtmempool_win64.a: $(OBJS_win64)
	$(AR) rs $@ $^

# --------------------------------------------------------------

rtmempool.o: rtmempool.c
	$(CC) $< $(BUILD_C_FLAGS) -c -o $@

# --------------------------------------------------------------

rtmempool_posix32.o: rtmempool.c
	$(CC) $< $(BUILD_C_FLAGS) $(32BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

rtmempool_posix64.o: rtmempool.c
	$(CC) $< $(BUILD_C_FLAGS) $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

rtmempool_win32.o: rtmempool.c
	$(CC) $< $(BUILD_C_FLAGS) -DPTW32_STATIC_LIB $(32BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

rtmempool_win64.o: rtmempool.c
	$(CC) $< $(BUILD_C_FLAGS) -DPTW32_STATIC_LIB $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

clean:
	rm -f *.o
