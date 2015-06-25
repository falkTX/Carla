#!/usr/bin/make -f
# Makefile for dgl #
# ---------------- #
# Created by falkTX
#

CWD=../..
MODULENAME=dgl
include ../Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += $(DGL_FLAGS) -Isrc

# needed by sofd right now, fix later
BUILD_CXX_FLAGS += -fpermissive

# needed by oui-blendish
BUILD_CXX_FLAGS += -Wno-unused-parameter

# ----------------------------------------------------------------------------------------------------------------------------

OBJS = \
	$(OBJDIR)/Application.cpp.o \
	$(OBJDIR)/Color.cpp.o \
	$(OBJDIR)/Geometry.cpp.o \
	$(OBJDIR)/Image.cpp.o \
	$(OBJDIR)/ImageWidgets.cpp.o \
	$(OBJDIR)/NanoVG.cpp.o \
	$(OBJDIR)/NanoWidgets.cpp.o \
	$(OBJDIR)/Widget.cpp.o

ifeq ($(MACOS),true)
OBJS += $(OBJDIR)/Window.mm.o
else
OBJS += $(OBJDIR)/Window.cpp.o
endif

# ----------------------------------------------------------------------------------------------------------------------------

all: $(MODULEDIR)/$(MODULENAME).a

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(MODULEDIR)/$(MODULENAME)*.a

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME).a: $(OBJS)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).a"
	@rm -f $@
	@$(AR) crs $@ $^

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.cpp.o: src/%.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(OBJDIR)/Window.mm.o: src/Window.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@

-include $(OBJS:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
