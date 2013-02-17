#!/usr/bin/make -f
# Makefile for carla tests #
# ------------------------ #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../backend -I../includes -I../utils
# BUILD_CXX_FLAGS += -I/opt/mingw32/include

TARGETS = CarlaString RtList Thread Print_

all: $(TARGETS) RUN

# --------------------------------------------------------------

CarlaString: CarlaString.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@

RtList: RtList.cpp ../libs/rtmempool.a
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -pthread -lpthread -o $@

Thread: Thread.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -pthread -lpthread -o $@

Print_: Print.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o Print # $@

GL_: GL.cpp
# 	$(CXX) $^ $(BUILD_CXX_FLAGS) -I../backend/engine/plugin -I../libs/distrho-plugin-toolkit $(LINK_FLAGS) -lgdi32 -lopengl32 -lole32 -luuid -lws2_32 -o GL # $@
	$(CXX) $^ $(BUILD_CXX_FLAGS) -I../backend/engine/plugin -I../libs/distrho-plugin-toolkit $(LINK_FLAGS) -lGL -lGLU -o GL # $@

RUN: $(TARGETS)
# 	./CarlaString && ./RtList && ./Thread
	./Print
# 	./GL

# --------------------------------------------------------------

clean:
	rm -f $(TARGETS)

debug:
	$(MAKE) DEBUG=true
