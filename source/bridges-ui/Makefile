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
OBJDIR    := $(CWD)/../build/bridges-plugin/Debug
MODULEDIR := $(CWD)/../build/modules/Debug
else
OBJDIR    := $(CWD)/../build/bridges-plugin/Release
MODULEDIR := $(CWD)/../build/modules/Release
endif

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS  += -DBUILD_BRIDGE -I. -I$(CWD)/backend -I$(CWD)/includes -I$(CWD)/modules -I$(CWD)/utils
BUILD_CXX_FLAGS  += $(LIBLO_FLAGS)

BUILD_LV2_FLAGS   = $(BUILD_CXX_FLAGS) -DBRIDGE_LV2
BUILD_VST2_FLAGS  = $(BUILD_CXX_FLAGS) -DBRIDGE_VST2

ifeq ($(CARLA_VESTIGE_HEADER),true)
BUILD_VST2_FLAGS += -DVESTIGE_HEADER
endif

LINK_FLAGS += $(JUCE_CORE_LIBS)
LINK_FLAGS += $(LIBLO_LIBS)

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_LV2_GTK2_FLAGS    = $(BUILD_LV2_FLAGS) -DBRIDGE_GTK2 -DBRIDGE_LV2_GTK2 $(shell pkg-config --cflags gtk+-2.0)
LINK_LV2_GTK2_FLAGS     = $(LINK_FLAGS) $(shell pkg-config --libs gtk+-2.0) -ldl

BUILD_LV2_GTK3_FLAGS    = $(BUILD_LV2_FLAGS) -DBRIDGE_GTK3 -DBRIDGE_LV2_GTK3 $(shell pkg-config --cflags gtk+-3.0)
LINK_LV2_GTK3_FLAGS     = $(LINK_FLAGS) $(shell pkg-config --libs gtk+-3.0) -ldl

BUILD_LV2_QT4_FLAGS     = $(BUILD_LV2_FLAGS) -DBRIDGE_QT4 -DBRIDGE_LV2_QT4 $(shell pkg-config --cflags QtCore QtGui) -I$(OBJDIR) -I$(CWD)/modules/theme
LINK_LV2_QT4_FLAGS      = $(LINK_FLAGS) $(shell pkg-config --libs QtCore QtGui) -ldl

BUILD_LV2_QT5_FLAGS     = $(BUILD_LV2_FLAGS) -DBRIDGE_QT5 -DBRIDGE_LV2_QT5 $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Widgets) -I$(OBJDIR) -I$(CWD)/modules/theme
LINK_LV2_QT5_FLAGS      = $(LINK_FLAGS) $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Widgets) -ldl

BUILD_LV2_X11_FLAGS     = $(BUILD_LV2_FLAGS) -DBRIDGE_X11 -DBRIDGE_LV2_X11 $(X11_FLAGS)
LINK_LV2_X11_FLAGS      = $(LINK_FLAGS) $(X11_LIBS) -ldl

BUILD_LV2_COCOA_FLAGS   = $(BUILD_LV2_FLAGS) -DBRIDGE_COCOA -DBRIDGE_LV2_COCOA -ObjC++
LINK_LV2_COCOA_FLAGS    = $(LINK_FLAGS) -framework Cocoa -framework IOKit -framework QuartzCore -ldl

BUILD_LV2_WINDOWS_FLAGS = $(BUILD_LV2_FLAGS) -DBRIDGE_HWND -DBRIDGE_LV2_HWND
LINK_LV2_WINDOWS_FLAGS  = $(LINK_FLAGS) -static -mwindows

BUILD_VST2_X11_FLAGS    = $(BUILD_VST2_FLAGS) -DBRIDGE_X11 -DBRIDGE_VST2_X11 $(X11_FLAGS)
LINK_VST2_X11_FLAGS     = $(LINK_FLAGS) $(X11_LIBS) -ldl

ifeq ($(LINUX),true)
LINK_LV2_GTK2_FLAGS += -lX11
LINK_LV2_GTK3_FLAGS += -lX11
LINK_LV2_QT4_FLAGS  += -lX11
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
TARGETS += ui_vst2-x11
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
	rm -f $(BINDIR)/$(MODULENAME)-lv2-* $(BINDIR)/$(MODULENAME)-vst2-*

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
ui_vst2-x11:    $(BINDIR)/$(MODULENAME)-vst2-x11

# ----------------------------------------------------------------------------------------------------------------------------
# Common libs

LIBS_LV2 = \
	$(MODULEDIR)/juce_core.a \
	$(MODULEDIR)/lilv.a

LIBS_LV2_JUCE = \
	$(LIBS_LV2) \
	$(MODULEDIR)/juce_data_structures.a \
	$(MODULEDIR)/juce_events.a \
	$(MODULEDIR)/juce_graphics.a \
	$(MODULEDIR)/juce_gui_basics.a

ifeq ($(MACOS),true)
LIBS_LV2_JUCE += \
	$(MODULEDIR)/juce_gui_extra.a
endif

LIBS_VST2 = \
	$(MODULEDIR)/juce_core.a

# ----------------------------------------------------------------------------------------------------------------------------
# Common objects

$(OBJDIR)/%.cpp.lv2.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (LV2)"
	@$(CXX) $< $(BUILD_LV2_FLAGS) -MMD -c -o $@

$(OBJDIR)/%.cpp.vst2.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (VST2)"
	@$(CXX) $< $(BUILD_VST2_FLAGS) -MMD -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------
# ui_lv2-gtk2

OBJS_LV2_GTK2 = \
	$(OBJDIR)/CarlaBridgeClient.cpp.lv2.o \
	$(OBJDIR)/CarlaBridgeOsc.cpp.lv2.o \
	$(OBJDIR)/CarlaBridgeToolkitGtk.cpp.lv2-gtk2.o \
	$(OBJDIR)/CarlaBridgeUI-LV2.cpp.lv2-gtk2.o

$(BINDIR)/$(MODULENAME)-lv2-gtk2: $(OBJS_LV2_GTK2) $(LIBS_LV2)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-gtk2"
	@$(CXX) $(OBJS_LV2_GTK2) $(LIBS_START) $(LIBS_LV2) $(LIBS_END) $(LINK_LV2_GTK2_FLAGS) -o $@

$(OBJDIR)/%.cpp.lv2-gtk2.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (LV2-GTK2)"
	@$(CXX) $< $(BUILD_LV2_GTK2_FLAGS) -MMD -c -o $@

-include $(OBJS_LV2_GTK2:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
# ui_lv2-gtk3

OBJS_LV2_GTK3 = \
	$(OBJDIR)/CarlaBridgeClient.cpp.lv2.o \
	$(OBJDIR)/CarlaBridgeOsc.cpp.lv2.o \
	$(OBJDIR)/CarlaBridgeToolkitGtk.cpp.lv2-gtk3.o \
	$(OBJDIR)/CarlaBridgeUI-LV2.cpp.lv2-gtk3.o

$(BINDIR)/$(MODULENAME)-lv2-gtk3: $(OBJS_LV2_GTK3) $(LIBS_LV2)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-gtk3"
	@$(CXX) $(OBJS_LV2_GTK3) $(LIBS_START) $(LIBS_LV2) $(LIBS_END) $(LINK_LV2_GTK3_FLAGS) -o $@

$(OBJDIR)/%.cpp.lv2-gtk3.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (LV2-GTK3)"
	@$(CXX) $< $(BUILD_LV2_GTK3_FLAGS) -MMD -c -o $@

-include $(OBJS_LV2_GTK3:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
# ui_lv2-qt4

OBJS_LV2_QT4 = \
	$(OBJDIR)/CarlaBridgeClient.cpp.lv2.o \
	$(OBJDIR)/CarlaBridgeOsc.cpp.lv2.o \
	$(OBJDIR)/CarlaBridgeToolkitQt.cpp.lv2-qt4.o \
	$(OBJDIR)/CarlaBridgeUI-LV2.cpp.lv2-qt4.o

$(BINDIR)/$(MODULENAME)-lv2-qt4: $(OBJS_LV2_QT4) $(LIBS_LV2) $(MODULEDIR)/theme.qt4.a
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-qt4"
	@$(CXX) $(OBJS_LV2_QT4) $(LIBS_START) $(LIBS_LV2) $(MODULEDIR)/theme.qt4.a $(LIBS_END) $(LINK_LV2_QT4_FLAGS) -o $@

$(OBJDIR)/%.cpp.lv2-qt4.o: %.cpp $(OBJDIR)/CarlaBridgeToolkitQt4.moc $(OBJDIR)/resources.qt4.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (LV2-QT4)"
	@$(CXX) $< $(BUILD_LV2_QT4_FLAGS) -MMD -c -o $@

$(OBJDIR)/CarlaBridgeToolkitQt4.moc: CarlaBridgeToolkitQt.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Generating CarlaBridgeToolkitQt4.moc"
	@$(MOC_QT4) $< -DMOC_PARSING -o $@

$(OBJDIR)/resources.qt4.cpp: ../../resources/resources-theme.qrc
	-@mkdir -p $(OBJDIR)
	@echo "Generating resources.qt4.cpp"
	@$(RCC_QT4) $< -o $@

-include $(OBJS_LV2_QT4:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
# ui_lv2-qt5

OBJS_LV2_QT5 = \
	$(OBJDIR)/CarlaBridgeClient.cpp.lv2.o \
	$(OBJDIR)/CarlaBridgeOsc.cpp.lv2.o \
	$(OBJDIR)/CarlaBridgeToolkitQt.cpp.lv2-qt5.o \
	$(OBJDIR)/CarlaBridgeUI-LV2.cpp.lv2-qt5.o

$(BINDIR)/$(MODULENAME)-lv2-qt5: $(OBJS_LV2_QT5) $(LIBS_LV2) $(MODULEDIR)/theme.qt5.a
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-qt5"
	@$(CXX) $(OBJS_LV2_QT5) $(LIBS_START) $(LIBS_LV2) $(MODULEDIR)/theme.qt5.a $(LIBS_END) $(LINK_LV2_QT5_FLAGS) -o $@

$(OBJDIR)/%.cpp.lv2-qt5.o: %.cpp $(OBJDIR)/CarlaBridgeToolkitQt5.moc $(OBJDIR)/resources.qt5.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (LV2-QT5)"
	@$(CXX) $< $(BUILD_LV2_QT5_FLAGS) -MMD -c -o $@

$(OBJDIR)/CarlaBridgeToolkitQt5.moc: CarlaBridgeToolkitQt.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Generating CarlaBridgeToolkitQt5.moc"
	@$(MOC_QT5) $< -DMOC_PARSING -o $@

$(OBJDIR)/resources.qt5.cpp: ../../resources/resources-theme.qrc
	-@mkdir -p $(OBJDIR)
	@echo "Generating resources.qt5.cpp"
	@$(RCC_QT5) $< -o $@

-include $(OBJS_LV2_QT5:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
# ui_lv2-x11

OBJS_LV2_X11 = \
	$(OBJDIR)/CarlaBridgeClient.cpp.lv2.o \
	$(OBJDIR)/CarlaBridgeOsc.cpp.lv2.o \
	$(OBJDIR)/CarlaBridgeToolkitPlugin.cpp.lv2-x11.o \
	$(OBJDIR)/CarlaBridgeUI-LV2.cpp.lv2-x11.o

$(BINDIR)/$(MODULENAME)-lv2-x11: $(OBJS_LV2_X11) $(LIBS_LV2)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-x11"
	@$(CXX) $(OBJS_LV2_X11) $(LIBS_START) $(LIBS_LV2) $(LIBS_END) $(LINK_LV2_X11_FLAGS) -o $@

$(OBJDIR)/%.cpp.lv2-x11.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (LV2-X11)"
	@$(CXX) $< $(BUILD_LV2_X11_FLAGS) -MMD -c -o $@

-include $(OBJS_LV2_X11:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
# ui_lv2-cocoa

OBJS_LV2_COCOA = \
	$(OBJDIR)/CarlaBridgeClient.cpp.lv2.o \
	$(OBJDIR)/CarlaBridgeOsc.cpp.lv2.o \
	$(OBJDIR)/CarlaBridgeToolkitPlugin.cpp.lv2-cocoa.o \
	$(OBJDIR)/CarlaBridgeUI-LV2.cpp.lv2-cocoa.o

$(BINDIR)/$(MODULENAME)-lv2-cocoa: $(OBJS_LV2_COCOA) $(LIBS_LV2_JUCE)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-cocoa"
	@$(CXX) $(OBJS_LV2_COCOA) $(LIBS_START) $(LIBS_LV2_JUCE) $(LIBS_END) $(LINK_LV2_COCOA_FLAGS) -o $@

$(OBJDIR)/%.cpp.lv2-cocoa.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (LV2-Cocoa)"
	@$(CXX) $< $(BUILD_LV2_COCOA_FLAGS) -MMD -c -o $@

-include $(OBJS_LV2_COCOA:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
# ui_lv2-windows

OBJS_LV2_WINDOWS = \
	$(OBJDIR)/CarlaBridgeClient.cpp.lv2.o \
	$(OBJDIR)/CarlaBridgeOsc.cpp.lv2.o \
	$(OBJDIR)/CarlaBridgeToolkitPlugin.cpp.lv2-windows.o \
	$(OBJDIR)/CarlaBridgeUI-LV2.cpp.lv2-windows.o

$(BINDIR)/$(MODULENAME)-lv2-windows.exe: $(OBJS_LV2_WINDOWS) $(LIBS_LV2_JUCE)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-windows.exe"
	@$(CXX) $(OBJS_LV2_WINDOWS) $(LIBS_START) $(LIBS_LV2_JUCE) $(LIBS_END) $(LINK_LV2_WINDOWS_FLAGS) -o $@

$(OBJDIR)/%.cpp.lv2-windows.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (LV2-Windows)"
	@$(CXX) $< $(BUILD_LV2_WINDOWS_FLAGS) -MMD -c -o $@

-include $(OBJS_LV2_WINDOWS:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
# ui_vst2-x11

OBJS_VST2_X11 = \
	$(OBJDIR)/CarlaBridgeClient.cpp.vst2.o \
	$(OBJDIR)/CarlaBridgeOsc.cpp.vst2.o \
	$(OBJDIR)/CarlaBridgeToolkitPlugin.cpp.vst2-x11.o \
	$(OBJDIR)/CarlaBridgeUI-VST2.cpp.vst2-x11.o

$(BINDIR)/$(MODULENAME)-vst2-x11: $(OBJS_VST2_X11) $(LIBS_VST2)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-vst2-x11"
	@$(CXX) $(OBJS_VST2_X11) $(LIBS_START) $(LIBS_VST2) $(LIBS_END) $(LINK_VST2_X11_FLAGS) -o $@

$(OBJDIR)/%.cpp.vst2-x11.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (VST2-X11)"
	@$(CXX) $< $(BUILD_VST2_X11_FLAGS) -MMD -c -o $@

-include $(OBJS_VST2_X11:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
