#!/usr/bin/make -f
# Makefile for carla-bridges-ui #
# ----------------------------- #
# Created by falkTX
#

CWD=..
MODULENAME=carla-bridge
include $(CWD)/Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

BINDIR    := $(CWD)/../bin

ifeq ($(DEBUG),true)
OBJDIR    := $(CWD)/../build/bridges-ui/Debug
MODULEDIR := $(CWD)/../build/modules/Debug
else
OBJDIR    := $(CWD)/../build/bridges-ui/Release
MODULEDIR := $(CWD)/../build/modules/Release
endif

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS  += -DBUILD_BRIDGE -DBUILD_BRIDGE_UI -I. -I$(CWD)/backend -I$(CWD)/includes -I$(CWD)/utils -isystem $(CWD)/modules
LINK_FLAGS       += $(JUCE_CORE_LIBS)

ifneq ($(HAIKU),true)
LINK_FLAGS       += -lpthread
endif

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_GTK2_FLAGS    = $(BUILD_CXX_FLAGS) -DBRIDGE_GTK2 $(shell pkg-config --cflags gtk+-2.0)
LINK_GTK2_FLAGS     = $(LINK_FLAGS) $(shell pkg-config --libs gtk+-2.0) -ldl

BUILD_GTK3_FLAGS    = $(BUILD_CXX_FLAGS) -DBRIDGE_GTK3 $(shell pkg-config --cflags gtk+-3.0)
LINK_GTK3_FLAGS     = $(LINK_FLAGS) $(shell pkg-config --libs gtk+-3.0) -ldl

BUILD_QT4_FLAGS     = $(BUILD_CXX_FLAGS) -DBRIDGE_QT4 $(shell pkg-config --cflags QtCore QtGui) -I$(OBJDIR) -I$(CWD)/theme
LINK_QT4_FLAGS      = $(LINK_FLAGS) $(shell pkg-config --libs QtCore QtGui) -ldl

BUILD_QT5_FLAGS     = $(BUILD_CXX_FLAGS) -DBRIDGE_QT5 $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Widgets) -I$(OBJDIR) -I$(CWD)/theme
LINK_QT5_FLAGS      = $(LINK_FLAGS) $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Widgets) -ldl

BUILD_X11_FLAGS     = $(BUILD_CXX_FLAGS) -DBRIDGE_X11 $(X11_FLAGS)
LINK_X11_FLAGS      = $(LINK_FLAGS) $(X11_LIBS) -ldl

BUILD_COCOA_FLAGS   = $(BUILD_CXX_FLAGS) -DBRIDGE_COCOA -ObjC++
LINK_COCOA_FLAGS    = $(LINK_FLAGS) -framework Cocoa -framework IOKit -framework QuartzCore -ldl

BUILD_WINDOWS_FLAGS = $(BUILD_CXX_FLAGS) -DBRIDGE_HWND
LINK_WINDOWS_FLAGS  = $(LINK_FLAGS) -static -mwindows

ifeq ($(LINUX),true)
ifeq ($(HAVE_X11),true)
LINK_GTK2_FLAGS += -lX11
LINK_GTK3_FLAGS += -lX11
LINK_QT4_FLAGS  += -lX11
endif
endif

# ----------------------------------------------------------------------------------------------------------------------------

ifeq ($(MACOS_OR_WIN32),true)
HAVE_QT4 = false
HAVE_QT5 = false
endif

ifeq ($(HAVE_GTK2),true)
TARGETS += ui_lv2-gtk2
endif

ifeq ($(HAVE_GTK3),true)
TARGETS += ui_lv2-gtk3
endif

ifeq ($(HAVE_QT4),true)
TARGETS += ui_lv2-qt4
endif

ifeq ($(HAVE_QT5),true)
TARGETS += ui_lv2-qt5
endif

ifeq ($(HAVE_X11),true)
TARGETS += ui_lv2-x11
endif

ifeq ($(MACOS),true)
TARGETS += ui_lv2-cocoa
endif

ifeq ($(WIN32),true)
TARGETS += ui_lv2-windows
endif

# ----------------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.cpp $(OBJDIR)/*.moc $(OBJDIR)/*.o
	rm -f $(BINDIR)/$(MODULENAME)-lv2-gtk*
	rm -f $(BINDIR)/$(MODULENAME)-lv2-qt*
	rm -f $(BINDIR)/$(MODULENAME)-lv2-cocoa
	rm -f $(BINDIR)/$(MODULENAME)-lv2-x11
	rm -f $(BINDIR)/$(MODULENAME)-lv2-windows

debug:
	$(MAKE) DEBUG=true

doxygen: carla_bridge.doxygen
	doxygen $<

# ----------------------------------------------------------------------------------------------------------------------------

ui_lv2-gtk2:    $(BINDIR)/$(MODULENAME)-lv2-gtk2
ui_lv2-gtk3:    $(BINDIR)/$(MODULENAME)-lv2-gtk3
ui_lv2-qt4:     $(BINDIR)/$(MODULENAME)-lv2-qt4
ui_lv2-qt5:     $(BINDIR)/$(MODULENAME)-lv2-qt5
ui_lv2-x11:     $(BINDIR)/$(MODULENAME)-lv2-x11
ui_lv2-cocoa:   $(BINDIR)/$(MODULENAME)-lv2-cocoa
ui_lv2-windows: $(BINDIR)/$(MODULENAME)-lv2-windows.exe

# ----------------------------------------------------------------------------------------------------------------------------
# Common libs

LIBS = \
	$(MODULEDIR)/juce_core.a \
	$(MODULEDIR)/lilv.a

LIBS_JUCE = \
	$(LIBS) \
	$(MODULEDIR)/juce_data_structures.a \
	$(MODULEDIR)/juce_events.a \
	$(MODULEDIR)/juce_graphics.a \
	$(MODULEDIR)/juce_gui_basics.a

ifeq ($(MACOS),true)
LIBS_JUCE += \
	$(MODULEDIR)/juce_gui_extra.a
endif

# ----------------------------------------------------------------------------------------------------------------------------
# Common objects

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------
# ui_lv2-gtk2

OBJS_GTK2 = \
	$(OBJDIR)/CarlaBridgeUI.cpp.o \
	$(OBJDIR)/CarlaBridgeUI-LV2.cpp-gtk2.o \
	$(OBJDIR)/CarlaBridgeToolkitGtk.cpp-gtk2.o

$(BINDIR)/$(MODULENAME)-lv2-gtk2: $(OBJS_GTK2) $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-gtk2"
	@$(CXX) $(OBJS_GTK2) $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_GTK2_FLAGS) -o $@

$(OBJDIR)/%.cpp-gtk2.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (GTK2)"
	@$(CXX) $< $(BUILD_GTK2_FLAGS) -c -o $@

-include $(OBJS_GTK2:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
# ui_lv2-gtk3

OBJS_GTK3 = \
	$(OBJDIR)/CarlaBridgeUI.cpp.o \
	$(OBJDIR)/CarlaBridgeUI-LV2.cpp-gtk3.o \
	$(OBJDIR)/CarlaBridgeToolkitGtk.cpp-gtk3.o

$(BINDIR)/$(MODULENAME)-lv2-gtk3: $(OBJS_GTK3) $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-gtk3"
	@$(CXX) $(OBJS_GTK3) $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_GTK3_FLAGS) -o $@

$(OBJDIR)/%.cpp-gtk3.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (GTK3)"
	@$(CXX) $< $(BUILD_GTK3_FLAGS) -c -o $@

-include $(OBJS_GTK3:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
# ui_lv2-qt4

OBJS_QT4 = \
	$(OBJDIR)/CarlaBridgeUI.cpp.o \
	$(OBJDIR)/CarlaBridgeUI-LV2.cpp-qt4.o \
	$(OBJDIR)/CarlaBridgeToolkitQt.cpp-qt4.o

$(BINDIR)/$(MODULENAME)-lv2-qt4: $(OBJS_QT4) $(LIBS) $(MODULEDIR)/theme.qt4.a
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-qt4"
	@$(CXX) $(OBJS_QT4) $(LIBS_START) $(LIBS) $(MODULEDIR)/theme.qt4.a $(LIBS_END) $(LINK_QT4_FLAGS) -o $@

$(OBJDIR)/%.cpp-qt4.o: %.cpp $(OBJDIR)/CarlaBridgeToolkitQt4.moc $(OBJDIR)/resources.qt4.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (QT4)"
	@$(CXX) $< $(BUILD_QT4_FLAGS) -c -o $@

$(OBJDIR)/CarlaBridgeToolkitQt4.moc: CarlaBridgeToolkitQt.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Generating CarlaBridgeToolkitQt4.moc"
	@$(MOC_QT4) $< -DMOC_PARSING -o $@

$(OBJDIR)/resources.qt4.cpp: ../../resources/resources-theme.qrc
	-@mkdir -p $(OBJDIR)
	@echo "Generating resources.qt4.cpp"
	@$(RCC_QT4) $< -o $@

-include $(OBJS_QT4:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
# ui_lv2-qt5

OBJS_QT5 = \
	$(OBJDIR)/CarlaBridgeUI.cpp.o \
	$(OBJDIR)/CarlaBridgeUI-LV2.cpp-qt5.o \
	$(OBJDIR)/CarlaBridgeToolkitQt.cpp-qt5.o

$(BINDIR)/$(MODULENAME)-lv2-qt5: $(OBJS_QT5) $(LIBS) $(MODULEDIR)/theme.qt5.a
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-qt5"
	@$(CXX) $(OBJS_QT5) $(LIBS_START) $(LIBS) $(MODULEDIR)/theme.qt5.a $(LIBS_END) $(LINK_QT5_FLAGS) -o $@

$(OBJDIR)/%.cpp-qt5.o: %.cpp $(OBJDIR)/CarlaBridgeToolkitQt5.moc $(OBJDIR)/resources.qt5.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (QT5)"
	@$(CXX) $< $(BUILD_QT5_FLAGS) -c -o $@

$(OBJDIR)/CarlaBridgeToolkitQt5.moc: CarlaBridgeToolkitQt.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Generating CarlaBridgeToolkitQt5.moc"
	@$(MOC_QT5) $< -DMOC_PARSING -o $@

$(OBJDIR)/resources.qt5.cpp: ../../resources/resources-theme.qrc
	-@mkdir -p $(OBJDIR)
	@echo "Generating resources.qt5.cpp"
	@$(RCC_QT5) $< -o $@

-include $(OBJS_QT5:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
# ui_lv2-x11

OBJS_X11 = \
	$(OBJDIR)/CarlaBridgeUI.cpp.o \
	$(OBJDIR)/CarlaBridgeUI-LV2.cpp-x11.o \
	$(OBJDIR)/CarlaBridgeToolkitPlugin.cpp-x11.o

$(BINDIR)/$(MODULENAME)-lv2-x11: $(OBJS_X11) $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-x11"
	@$(CXX) $(OBJS_X11) $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_X11_FLAGS) -o $@

$(OBJDIR)/%.cpp-x11.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (X11)"
	@$(CXX) $< $(BUILD_X11_FLAGS) -c -o $@

-include $(OBJS_X11:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
# ui_lv2-cocoa

OBJS_COCOA = \
	$(OBJDIR)/CarlaBridgeUI.cpp.o \
	$(OBJDIR)/CarlaBridgeUI-LV2.cpp-cocoa.o \
	$(OBJDIR)/CarlaBridgeToolkitPlugin.cpp-cocoa.o

$(BINDIR)/$(MODULENAME)-lv2-cocoa: $(OBJS_COCOA) $(LIBS_JUCE)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-cocoa"
	@$(CXX) $(OBJS_COCOA) $(LIBS_START) $(LIBS_JUCE) $(LIBS_END) $(LINK_COCOA_FLAGS) -o $@

$(OBJDIR)/%.cpp-cocoa.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (Cocoa)"
	@$(CXX) $< $(BUILD_COCOA_FLAGS) -c -o $@

-include $(OBJS_COCOA:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
# ui_lv2-windows

OBJS_WINDOWS = \
	$(OBJDIR)/CarlaBridgeUI.cpp.o \
	$(OBJDIR)/CarlaBridgeUI-LV2.cpp-windows.o \
	$(OBJDIR)/CarlaBridgeToolkitPlugin.cpp-windows.o

$(BINDIR)/$(MODULENAME)-lv2-windows.exe: $(OBJS_WINDOWS) $(LIBS_JUCE)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-windows.exe"
	@$(CXX) $(OBJS_WINDOWS) $(LIBS_START) $(LIBS_JUCE) $(LIBS_END) $(LINK_WINDOWS_FLAGS) -o $@

$(OBJDIR)/%.cpp-windows.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (Windows)"
	@$(CXX) $< $(BUILD_WINDOWS_FLAGS) -c -o $@

-include $(OBJS_WINDOWS:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
