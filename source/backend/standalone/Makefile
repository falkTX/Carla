#!/usr/bin/make -f
# Makefile for carla-standalone #
# --------------------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

# LINK_FLAGS      += -shared
LINK_FLAGS      += -Wl,-rpath,. -L.. -lcarla_engine -lcarla_native -lcarla_plugin

# --------------------------------------------------------------

OBJS  = \
     carla_standalone.cpp.o

TARGET = ../carla_standalone
#lib*.so

# --------------------------------------------------------------

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

# --------------------------------------------------------------

%.cpp.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(TARGET): $(OBJS)
	$(CXX) $^ $(LINK_FLAGS) -o $@
