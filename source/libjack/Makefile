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

BUILD_CXX_FLAGS += -I$(CWD) -I$(CWD)/backend -I$(CWD)/includes -I$(CWD)/modules -I$(CWD)/utils
LINK_FLAGS      += $(MODULEDIR)/juce_core.a
LINK_FLAGS      += $(JUCE_CORE_LIBS)

# ----------------------------------------------------------------------------------------------------------------------

OBJS    =
TARGETS =

ifeq ($(LINUX),true)
OBJS    = \
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
	$(OBJDIR)/libjack_time.cpp.o \
	$(OBJDIR)/libjack_transport.cpp.o
TARGET = $(BINDIR)/jack/libjack.so.0
endif

# ----------------------------------------------------------------------------------------------------------------------

all: $(TARGET)

# ----------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------

$(TARGET): $(OBJS)
	-@mkdir -p $(BINDIR)/jack
	@echo "Linking libjack.so.0"
	@$(CXX) $^ $(SHARED) $(LINK_FLAGS) $(LIBJACK_LIBS) -o $@

# ----------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------

-include $(OBJS:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------
