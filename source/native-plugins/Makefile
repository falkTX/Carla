#!/usr/bin/make -f
# Makefile for native-plugins #
# --------------------------- #
# Created by falkTX
#

CWD=..
CWDE=..
MODULENAME=native-plugins
include ../modules/Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_C_FLAGS   += -I..
BUILD_CXX_FLAGS += -I.. -I$(CWD)/modules

# ---------------------------------------------------------------------------------------------------------------------
# Set targets

TARGETS = \
	$(MODULEDIR)/$(MODULENAME).all.a \
	$(MODULEDIR)/$(MODULENAME).base.a

# ---------------------------------------------------------------------------------------------------------------------
# Set objects

OBJS = \
	$(OBJDIR)/bypass.c.o \
	$(OBJDIR)/lfo.c.o \
	$(OBJDIR)/midi-channel-filter.c.o \
	$(OBJDIR)/midi-gain.c.o \
	$(OBJDIR)/midi-join.c.o \
	$(OBJDIR)/midi-split.c.o \
	$(OBJDIR)/midi-through.c.o \
	$(OBJDIR)/midi-transpose.c.o \
	$(OBJDIR)/audio-file.cpp.o \
	$(OBJDIR)/bigmeter.cpp.o \
	$(OBJDIR)/midi-file.cpp.o \
	$(OBJDIR)/midi-pattern.cpp.o \
	$(OBJDIR)/notes.cpp.o

OBJS_base = \
	$(OBJDIR)/_all.base.c.o \
	$(OBJDIR)/_data.base.cpp.o \
	$(OBJS)

OBJS_all = \
	$(OBJDIR)/_all.all.c.o \
	$(OBJDIR)/_data.all.cpp.o \
	$(OBJS)

# ---------------------------------------------------------------------------------------------------------------------
# Include external plugins, if present

ifneq (,$(wildcard external/Makefile))
include external/Makefile
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(MODULEDIR)/$(MODULENAME)*.a $(ZYN_UI_FILES_H) $(ZYN_UI_FILES_CPP) $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME).all.a: $(OBJS_all)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).all.a"
	@rm -f $@
	@$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).base.a: $(OBJS_base)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).base.a"
	@rm -f $@
	@$(AR) crs $@ $^

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.c.o: %.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CC) $< $(BUILD_C_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJS_all:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
