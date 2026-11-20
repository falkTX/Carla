#!/usr/bin/make -f
# Makefile for dgl #
# ---------------- #
# Created by falkTX
#

CWD=../..
MODULENAME=dgl
include ../Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += $(DGL_FLAGS) -Isrc
BUILD_CXX_FLAGS += -Isrc/pugl-upstream/include

ifneq ($(MACOS),true)
WINE_FLAGS  = -I/usr/include/wine/wine/windows
WINE_FLAGS += -I/usr/include/wine-development/windows
WINE_FLAGS += -I/opt/wine-devel/include/wine/windows
endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS = \
	$(OBJDIR)/pugl.cpp.o \
	$(OBJDIR)/Application.cpp.o \
	$(OBJDIR)/ApplicationPrivateData.cpp.o \
	$(OBJDIR)/Color.cpp.o \
	$(OBJDIR)/EventHandlers.cpp.o \
	$(OBJDIR)/Geometry.cpp.o \
	$(OBJDIR)/ImageBase.cpp.o \
	$(OBJDIR)/ImageBaseWidgets.cpp.o \
	$(OBJDIR)/NanoVG.cpp.o \
	$(OBJDIR)/OpenGL.cpp.o \
	$(OBJDIR)/OpenGL2.cpp.o \
	$(OBJDIR)/SubWidget.cpp.o \
	$(OBJDIR)/SubWidgetPrivateData.cpp.o \
	$(OBJDIR)/TopLevelWidget.cpp.o \
	$(OBJDIR)/TopLevelWidgetPrivateData.cpp.o \
	$(OBJDIR)/Widget.cpp.o \
	$(OBJDIR)/WidgetPrivateData.cpp.o \
	$(OBJDIR)/Window.cpp.o \
	$(OBJDIR)/WindowPrivateData.cpp.o

OBJS_wine = \
	$(OBJDIR)/pugl.cpp-wine.o \
	$(OBJDIR)/Application.cpp-wine.o \
	$(OBJDIR)/ApplicationPrivateData.cpp-wine.o \
	$(OBJDIR)/Color.cpp-wine.o \
	$(OBJDIR)/EventHandlers.cpp-wine.o \
	$(OBJDIR)/Geometry.cpp-wine.o \
	$(OBJDIR)/ImageBase.cpp-wine.o \
	$(OBJDIR)/ImageBaseWidgets.cpp-wine.o \
	$(OBJDIR)/OpenGL.cpp-wine.o \
	$(OBJDIR)/OpenGL2.cpp-wine.o \
	$(OBJDIR)/SubWidget.cpp-wine.o \
	$(OBJDIR)/SubWidgetPrivateData.cpp-wine.o \
	$(OBJDIR)/TopLevelWidget.cpp-wine.o \
	$(OBJDIR)/TopLevelWidgetPrivateData.cpp-wine.o \
	$(OBJDIR)/Widget.cpp-wine.o \
	$(OBJDIR)/WidgetPrivateData.cpp-wine.o \
	$(OBJDIR)/Window.cpp-wine.o \
	$(OBJDIR)/WindowPrivateData.cpp-wine.o

# ---------------------------------------------------------------------------------------------------------------------

all:  $(MODULEDIR)/$(MODULENAME).a
wine: $(MODULEDIR)/$(MODULENAME).wine.a

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(MODULEDIR)/$(MODULENAME)*.a $(MODULEDIR)/$(MODULENAME).wine.a

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME).a: $(OBJS)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).wine.a: $(OBJS_wine)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).wine.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(MACOS),true)
$(OBJDIR)/pugl.cpp.o: src/pugl.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@
endif

$(OBJDIR)/%.cpp.o: src/%.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(OBJDIR)/%.cpp-wine.o: src/%.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (Wine)"
	$(SILENT)$(WINECC) $< $(BUILD_CXX_FLAGS) $(WINE_FLAGS) -c -o $@

-include $(OBJS:%.o=%.d)
-include $(OBJS_wine:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
