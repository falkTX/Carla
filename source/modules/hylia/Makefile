#!/usr/bin/make -f
# Makefile for hylia #
# ------------------ #
# Created by falkTX
#

CWD=../..
MODULENAME=hylia
include ../Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += $(HYLIA_FLAGS) -I$(CWD) -I$(CWD)/modules -Ilink
BUILD_CXX_FLAGS += -Wno-missing-field-initializers -Wno-multichar -Wno-uninitialized -Wno-unknown-pragmas
# -Wno-unused-parameter
# -Wno-unused-variable

# ---------------------------------------------------------------------------------------------------------------------

OBJS = $(OBJDIR)/hylia.cpp.o

# ---------------------------------------------------------------------------------------------------------------------

all: $(MODULEDIR)/$(MODULENAME).a

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(MODULEDIR)/$(MODULENAME)*.a

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME).a: $(OBJS)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

-include $(OBJS:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
