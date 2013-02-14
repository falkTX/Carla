#!/usr/bin/make -f
# Makefile for carla tests #
# ------------------------ #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../backend -I../includes -I../utils
# BUILD_CXX_FLAGS += -I/opt/mingw32/include
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore)
LINK_FLAGS      += $(shell pkg-config --libs QtCore)

TARGETS = CarlaString RtList Thread GL_

all: $(TARGETS) RUN

# --------------------------------------------------------------

CarlaString: CarlaString.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@

RtList: RtList.cpp ../libs/rtmempool.a
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@

Thread: Thread.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -pthread -lpthread -o $@

GL_: GL.cpp
# 	$(CXX) $^ $(BUILD_CXX_FLAGS) -I../backend/engine/plugin -I../libs/distrho-plugin-toolkit $(LINK_FLAGS) -lgdi32 -lopengl32 -lole32 -luuid -lws2_32 -o GL # $@
	$(CXX) $^ $(BUILD_CXX_FLAGS) -I../backend/engine/plugin -I../libs/distrho-plugin-toolkit $(LINK_FLAGS) -lGL -lGLU -o GL # $@

RUN: $(TARGETS)
# 	./CarlaString && ./RtList && ./Thread
# 	./GL

# --------------------------------------------------------------

clean:
	rm -f $(TARGETS)

debug:
	$(MAKE) DEBUG=true
