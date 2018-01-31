#!/usr/bin/make -f
# Makefile for libjack #
# -------------------- #
# Created by falkTX
#

CWD=..
include $(CWD)/Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------

BINDIR    := $(CWD)/../bin

ifeq ($(DEBUG),true)
OBJDIR    := $(CWD)/../build/libjack/Debug
MODULEDIR := $(CWD)/../build/modules/Debug
else
OBJDIR    := $(CWD)/../build/libjack/Release
MODULEDIR := $(CWD)/../build/modules/Release
endif

# ----------------------------------------------------------------------------------------------------------------------

BUILD_C_FLAGS   += -I$(CWD) -I$(CWD)/includes
BUILD_CXX_FLAGS += -I$(CWD) -I$(CWD)/backend -I$(CWD)/includes -I$(CWD)/utils
LINK_FLAGS      += -lpthread -lrt

# ----------------------------------------------------------------------------------------------------------------------

ifeq ($(LINUX),true)
OBJS = \
	$(OBJDIR)/libjack.cpp.o \
	$(OBJDIR)/libjack_base.cpp.o \
	$(OBJDIR)/libjack_callbacks.cpp.o \
	$(OBJDIR)/libjack_client.cpp.o \
	$(OBJDIR)/libjack_error.cpp.o \
	$(OBJDIR)/libjack_latency.cpp.o \
	$(OBJDIR)/libjack_non-callback.cpp.o \
	$(OBJDIR)/libjack_metadata.cpp.o \
	$(OBJDIR)/libjack_midi.cpp.o \
	$(OBJDIR)/libjack_ports.cpp.o \
	$(OBJDIR)/libjack_port-searching.cpp.o \
	$(OBJDIR)/libjack_server-control.cpp.o \
	$(OBJDIR)/libjack_stats.cpp.o \
	$(OBJDIR)/libjack_time.cpp.o \
	$(OBJDIR)/libjack_transport.cpp.o \
	$(OBJDIR)/ringbuffer.c.o
TARGET = $(BINDIR)/jack/libjack.so.0
else
OBJS =
endif

# ----------------------------------------------------------------------------------------------------------------------

all: $(TARGET)

# ----------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(TARGET)

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------

$(TARGET): $(OBJS)
	-@mkdir -p $(BINDIR)/jack
	@echo "Linking libjack.so.0"
	@$(CXX) $^ $(SHARED) $(LINK_FLAGS) $(LIBJACK_LIBS) -o $@

# ----------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.c.o: %.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CC) $< $(BUILD_C_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------

-include $(OBJS:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------
