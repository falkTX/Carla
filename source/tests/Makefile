#!/usr/bin/make -f
# Makefile for carla tests #
# ------------------------ #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../backend -I../includes -I../utils
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore)
LINK_FLAGS      += $(shell pkg-config --libs QtCore)

TARGETS = CarlaString RtList Thread

all: $(TARGETS) RUN

# --------------------------------------------------------------

CarlaString: CarlaString.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@

RtList: RtList.cpp ../libs/rtmempool.a
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@

Thread_: Thread.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -pthread -lpthread -o $@

RUN: $(TARGETS)
	./CarlaString && ./RtList && ./Thread

# --------------------------------------------------------------

clean:
	rm -f $(TARGETS)

debug:
	$(MAKE) DEBUG=true
