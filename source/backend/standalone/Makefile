#!/usr/bin/make -f
# Makefile for carla-standalone #
# ----------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

TARGETS = \
	../carla_standalone.a

# --------------------------------------------------------------

all: $(TARGETS)

# --------------------------------------------------------------

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

../carla_standalone.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

# --------------------------------------------------------------

CarlaStandalone.cpp.o: CarlaStandalone.cpp $(CARLA_STANDALONE_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) -c -o $@

# --------------------------------------------------------------
