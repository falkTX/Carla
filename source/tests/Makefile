#!/usr/bin/make -f
# Makefile for carla tests #
# ------------------------ #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../backend -I../includes -I../libs -I../theme -I../utils -Wall -Wextra
# BUILD_CXX_FLAGS += -isystem ../libs/juce
# BUILD_CXX_FLAGS += -DWANT_JACK -DWANT_LADSPA -DWANT_DSSI
BUILD_CXX_FLAGS += -isystem /usr/include/qt4
# BUILD_CXX_FLAGS += -isystem ../backend/engine/rtaudio-4.0.11
# BUILD_CXX_FLAGS += -I/opt/mingw32/include

ANSI_CXX_FLAGS   = -ansi -pedantic -pedantic-errors -Wunused-parameter -Wuninitialized -Wno-vla
ANSI_CXX_FLAGS  += -Wcast-qual -Wconversion -Wsign-conversion -Wlogical-op -Waggregate-return
ANSI_CXX_FLAGS  += -std=c++11 -Wzero-as-null-pointer-constant
ANSI_CXX_FLAGS  += -DBUILD_ANSI_TEST -DVESTIGE_HEADER -fPIC

TARGETS = ANSI CarlaString RtList Print Utils

all: $(TARGETS) RUN

# --------------------------------------------------------------

ANSI: ANSI.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(ANSI_CXX_FLAGS) $(LINK_FLAGS) -o $@

CarlaString: CarlaString.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@

RtList: RtList.cpp ../libs/rtmempool.a ../utils/RtList.hpp
	$(CXX) RtList.cpp ../libs/rtmempool.a $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -pthread -lpthread -o $@

Print: Print.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@

Utils: Utils.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@

Rewire.exe: Rewire.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -lversion -o $@

RUN: $(TARGETS)
# 	valgrind ./Base64
# 	./ANSI
# 	./CarlaString && ./RtList && ./Thread
# 	./Print
# 	./Base64
# 	./GL

# --------------------------------------------------------------

clean:
	rm -f $(TARGETS)

debug:
	$(MAKE) DEBUG=true
