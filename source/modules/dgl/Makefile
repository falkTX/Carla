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

# ----------------------------------------------------------------------------------------------------------------------------

OBJS = \
	$(OBJDIR)/Application.cpp.o \
	$(OBJDIR)/Color.cpp.o \
	$(OBJDIR)/Geometry.cpp.o \
	$(OBJDIR)/Image.cpp.o \
	$(OBJDIR)/ImageWidgets.cpp.o \
	$(OBJDIR)/NanoVG.cpp.o \
	$(OBJDIR)/Widget.cpp.o

ifeq ($(MACOS),true)
OBJS += $(OBJDIR)/Window.mm.o
else
OBJS += $(OBJDIR)/Window.cpp.o
endif

OBJS_wine = \
	$(OBJDIR)/Application.cpp-wine.o \
	$(OBJDIR)/Color.cpp-wine.o \
	$(OBJDIR)/Geometry.cpp-wine.o \
	$(OBJDIR)/Image.cpp-wine.o \
	$(OBJDIR)/ImageWidgets.cpp-wine.o \
	$(OBJDIR)/Widget.cpp-wine.o \
	$(OBJDIR)/Window.cpp-wine.o

# ----------------------------------------------------------------------------------------------------------------------------

all:  $(MODULEDIR)/$(MODULENAME).a
wine: $(MODULEDIR)/$(MODULENAME).wine.a

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(MODULEDIR)/$(MODULENAME)*.a $(MODULEDIR)/$(MODULENAME).wine.a

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME).a: $(OBJS)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).a"
	@rm -f $@
	@$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).wine.a: $(OBJS_wine)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).wine.a"
	@rm -f $@
	@$(AR) crs $@ $^

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.cpp.o: src/%.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(OBJDIR)/%.cpp-wine.o: src/%.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (Wine)"
	@$(WINECC) $< $(BUILD_CXX_FLAGS) -c -o $@

$(OBJDIR)/Window.mm.o: src/Window.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@

-include $(OBJS:%.o=%.d)
-include $(OBJS_wine:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
