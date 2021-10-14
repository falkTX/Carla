#!/usr/bin/make -f
# Makefile for jsusfx #
# ------------------- #
# Created by falkTX
#

CWD=../..
MODULENAME=jsusfx
include ../Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += $(JSUSFX_FLAGS)
BUILD_CXX_FLAGS += -Wno-unused-parameter
BUILD_CXX_FLAGS += -Wno-unused-function
BUILD_CXX_FLAGS += -Wno-ignored-attributes
BUILD_CXX_FLAGS += -Wno-sign-compare
BUILD_CXX_FLAGS += -Wno-maybe-uninitialized

# ---------------------------------------------------------------------------------------------------------------------

OBJS  = $(OBJDIR)/source/jsusfx.cpp.o
OBJS += $(OBJDIR)/source/jsusfx_file.cpp.o
OBJS += $(OBJDIR)/source/jsusfx_gfx.cpp.o
OBJS += $(OBJDIR)/source/jsusfx_serialize.cpp.o
OBJS += $(OBJDIR)/source/riff.cpp.o

# ---------------------------------------------------------------------------------------------------------------------

all: $(MODULEDIR)/$(MODULENAME).a

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/source/*.o $(MODULEDIR)/$(MODULENAME)*.a

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME).a: $(OBJS)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).a"
	@rm -f $@
	@$(AR) crs $@ $^

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(dir $@)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

-include $(OBJS:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
