#!/usr/bin/make -f
# Makefile for carla-bridges-ui #
# ----------------------------- #
# Created by falkTX
#

CWD=..
MODULENAME=carla-bridge
include $(CWD)/Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

BINDIR    := $(CWD)/../bin

ifeq ($(DEBUG),true)
OBJDIR    := $(CWD)/../build/bridges-ui/Debug
MODULEDIR := $(CWD)/../build/modules/Debug
else
OBJDIR    := $(CWD)/../build/bridges-ui/Release
MODULEDIR := $(CWD)/../build/modules/Release
endif

# ---------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS  += -DBUILD_BRIDGE -DBUILD_BRIDGE_UI -I. -I$(CWD)/backend -I$(CWD)/includes -I$(CWD)/modules -I$(CWD)/utils
LINK_FLAGS       += $(WATER_LIBS)

ifneq ($(WASM),true)
ifeq ($(HAIKU),true)
LINK_FLAGS       += -lpthread
else
LINK_FLAGS       += -pthread
endif
endif

# ---------------------------------------------------------------------------------------------------------------------

BUILD_GTK2_FLAGS    = $(BUILD_CXX_FLAGS) -DBRIDGE_GTK2 $(X11_FLAGS)
LINK_GTK2_FLAGS     = $(LINK_FLAGS) $(X11_LIBS) $(LIBDL_LIBS)

BUILD_GTK3_FLAGS    = $(BUILD_CXX_FLAGS) -DBRIDGE_GTK3 $(X11_FLAGS)
LINK_GTK3_FLAGS     = $(LINK_FLAGS) $(X11_LIBS) $(LIBDL_LIBS)

BUILD_QT4_FLAGS     = $(BUILD_CXX_FLAGS) -DBRIDGE_QT4 $(shell pkg-config --cflags QtCore QtGui) $(X11_FLAGS) -I$(OBJDIR) -I$(CWD)/theme -Wno-unused-variable
LINK_QT4_FLAGS      = $(LINK_FLAGS) $(shell pkg-config --libs QtCore QtGui) $(X11_LIBS) $(LIBDL_LIBS)

BUILD_QT5_FLAGS     = $(BUILD_CXX_FLAGS) -DBRIDGE_QT5 $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Widgets) -I$(OBJDIR) -I$(CWD)/theme
LINK_QT5_FLAGS      = $(LINK_FLAGS) $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Widgets) $(LIBDL_LIBS)

BUILD_X11_FLAGS     = $(BUILD_CXX_FLAGS) -DBRIDGE_X11 $(X11_FLAGS)
LINK_X11_FLAGS      = $(LINK_FLAGS) $(X11_LIBS) $(LIBDL_LIBS)

BUILD_COCOA_FLAGS   = $(BUILD_CXX_FLAGS) -DBRIDGE_COCOA -ObjC++
LINK_COCOA_FLAGS    = $(LINK_FLAGS) -framework Cocoa $(LIBDL_LIBS)

BUILD_WINDOWS_FLAGS = $(BUILD_CXX_FLAGS) -DBRIDGE_HWND
LINK_WINDOWS_FLAGS  = $(LINK_FLAGS) -static -mwindows

ifeq ($(MACOS),true)
BUILD_GTK2_FLAGS  += -ObjC++
BUILD_GTK3_FLAGS  += -ObjC++
endif

ifeq ($(TESTBUILD),true)
BUILD_QT4_FLAGS  += -isystem /usr/include/qt4
BUILD_QT5_FLAGS  += -isystem /usr/include/qt5
BUILD_QT5_FLAGS  += -isystem /usr/include/x86_64-linux-gnu/qt5
endif

# ---------------------------------------------------------------------------------------------------------------------

TARGETS += ui_lv2-gtk2
TARGETS += ui_lv2-gtk3

ifneq ($(MACOS_OR_WINDOWS),true)
ifeq ($(HAVE_QT4),true)
TARGETS += ui_lv2-qt4
endif

ifeq ($(HAVE_QT5),true)
TARGETS += ui_lv2-qt5
endif

ifeq ($(HAVE_X11),true)
TARGETS += ui_lv2-x11
endif
endif

ifeq ($(MACOS),true)
TARGETS += ui_lv2-cocoa
endif

ifeq ($(WINDOWS),true)
TARGETS += ui_lv2-windows
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ---------------------------------------------------------------------------------------------------------------------

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

# ---------------------------------------------------------------------------------------------------------------------

ui_lv2-gtk2:    $(BINDIR)/$(MODULENAME)-lv2-gtk2$(APP_EXT)
ui_lv2-gtk3:    $(BINDIR)/$(MODULENAME)-lv2-gtk3$(APP_EXT)
ui_lv2-qt4:     $(BINDIR)/$(MODULENAME)-lv2-qt4$(APP_EXT)
ui_lv2-qt5:     $(BINDIR)/$(MODULENAME)-lv2-qt5$(APP_EXT)
ui_lv2-x11:     $(BINDIR)/$(MODULENAME)-lv2-x11$(APP_EXT)
ui_lv2-cocoa:   $(BINDIR)/$(MODULENAME)-lv2-cocoa$(APP_EXT)
ui_lv2-windows: $(BINDIR)/$(MODULENAME)-lv2-windows$(APP_EXT)

# ---------------------------------------------------------------------------------------------------------------------
# Common libs

LIBS = \
	$(MODULEDIR)/lilv.a \
	$(MODULEDIR)/water.files.a

# ---------------------------------------------------------------------------------------------------------------------
# Common objects

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------
# ui_lv2-gtk2

OBJS_GTK2 = \
	$(OBJDIR)/CarlaBridgeFormat.cpp.o \
	$(OBJDIR)/CarlaBridgeFormatLV2.cpp-gtk2.o \
	$(OBJDIR)/CarlaBridgeToolkitGtk.cpp-gtk2.o

$(BINDIR)/$(MODULENAME)-lv2-gtk2$(APP_EXT): $(OBJS_GTK2) $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-gtk2$(APP_EXT)"
	$(SILENT)$(CXX) $(OBJS_GTK2) $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_GTK2_FLAGS) -o $@

$(OBJDIR)/%.cpp-gtk2.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (GTK2)"
	$(SILENT)$(CXX) $< $(BUILD_GTK2_FLAGS) -c -o $@

-include $(OBJS_GTK2:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
# ui_lv2-gtk3

OBJS_GTK3 = \
	$(OBJDIR)/CarlaBridgeFormat.cpp.o \
	$(OBJDIR)/CarlaBridgeFormatLV2.cpp-gtk3.o \
	$(OBJDIR)/CarlaBridgeToolkitGtk.cpp-gtk3.o

$(BINDIR)/$(MODULENAME)-lv2-gtk3$(APP_EXT): $(OBJS_GTK3) $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-gtk3$(APP_EXT)"
	$(SILENT)$(CXX) $(OBJS_GTK3) $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_GTK3_FLAGS) -o $@

$(OBJDIR)/%.cpp-gtk3.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (GTK3)"
	$(SILENT)$(CXX) $< $(BUILD_GTK3_FLAGS) -c -o $@

-include $(OBJS_GTK3:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
# ui_lv2-qt4

OBJS_QT4 = \
	$(OBJDIR)/CarlaBridgeFormat.cpp.o \
	$(OBJDIR)/CarlaBridgeFormatLV2.cpp-qt4.o \
	$(OBJDIR)/CarlaBridgeToolkitQt.cpp-qt4.o

$(BINDIR)/$(MODULENAME)-lv2-qt4$(APP_EXT): $(OBJS_QT4) $(LIBS) $(MODULEDIR)/theme.qt4.a
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-qt4$(APP_EXT)"
	$(SILENT)$(CXX) $(OBJS_QT4) $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(MODULEDIR)/theme.qt4.a $(LINK_QT4_FLAGS) -o $@

$(OBJDIR)/%.cpp-qt4.o: %.cpp $(OBJDIR)/CarlaBridgeToolkitQt4.moc $(OBJDIR)/resources.qt4.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (QT4)"
	$(SILENT)$(CXX) $< $(BUILD_QT4_FLAGS) -c -o $@

$(OBJDIR)/CarlaBridgeToolkitQt4.moc: CarlaBridgeToolkitQt.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Generating CarlaBridgeToolkitQt4.moc"
	$(SILENT)$(MOC_QT4) $< -DMOC_PARSING -o $@

$(OBJDIR)/resources.qt4.cpp: ../../resources/resources-theme.qrc
	-@mkdir -p $(OBJDIR)
	@echo "Generating resources.qt4.cpp"
	$(SILENT)$(RCC_QT4) $< -o $@

-include $(OBJS_QT4:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
# ui_lv2-qt5

OBJS_QT5 = \
	$(OBJDIR)/CarlaBridgeFormat.cpp.o \
	$(OBJDIR)/CarlaBridgeFormatLV2.cpp-qt5.o \
	$(OBJDIR)/CarlaBridgeToolkitQt.cpp-qt5.o

$(BINDIR)/$(MODULENAME)-lv2-qt5$(APP_EXT): $(OBJS_QT5) $(LIBS) $(MODULEDIR)/theme.qt5.a
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-qt5$(APP_EXT)"
	$(SILENT)$(CXX) $(OBJS_QT5) $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(MODULEDIR)/theme.qt5.a $(LINK_QT5_FLAGS) -o $@

$(OBJDIR)/%.cpp-qt5.o: %.cpp $(OBJDIR)/CarlaBridgeToolkitQt5.moc $(OBJDIR)/resources.qt5.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (QT5)"
	$(SILENT)$(CXX) $< $(BUILD_QT5_FLAGS) -c -o $@

$(OBJDIR)/CarlaBridgeToolkitQt5.moc: CarlaBridgeToolkitQt.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Generating CarlaBridgeToolkitQt5.moc"
	$(SILENT)$(MOC_QT5) $< -DMOC_PARSING -o $@

$(OBJDIR)/resources.qt5.cpp: ../../resources/resources-theme.qrc
	-@mkdir -p $(OBJDIR)
	@echo "Generating resources.qt5.cpp"
	$(SILENT)$(RCC_QT5) $< -o $@

-include $(OBJS_QT5:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
# ui_lv2-x11

OBJS_X11 = \
	$(OBJDIR)/CarlaBridgeFormat.cpp.o \
	$(OBJDIR)/CarlaBridgeFormatLV2.cpp-x11.o \
	$(OBJDIR)/CarlaBridgeToolkitNative.cpp-x11.o

$(BINDIR)/$(MODULENAME)-lv2-x11$(APP_EXT): $(OBJS_X11) $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-x11$(APP_EXT)"
	$(SILENT)$(CXX) $(OBJS_X11) $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_X11_FLAGS) -o $@

$(OBJDIR)/%.cpp-x11.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (X11)"
	$(SILENT)$(CXX) $< $(BUILD_X11_FLAGS) -c -o $@

-include $(OBJS_X11:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
# ui_lv2-cocoa

OBJS_COCOA = \
	$(OBJDIR)/CarlaBridgeFormat.cpp.o \
	$(OBJDIR)/CarlaBridgeFormatLV2.cpp-cocoa.o \
	$(OBJDIR)/CarlaBridgeToolkitNative.cpp-cocoa.o

$(BINDIR)/$(MODULENAME)-lv2-cocoa$(APP_EXT): $(OBJS_COCOA) $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-cocoa$(APP_EXT)"
	$(SILENT)$(CXX) $(OBJS_COCOA) $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_COCOA_FLAGS) -o $@

$(OBJDIR)/%.cpp-cocoa.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (Cocoa)"
	$(SILENT)$(CXX) $< $(BUILD_COCOA_FLAGS) -c -o $@

-include $(OBJS_COCOA:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
# ui_lv2-windows

OBJS_WINDOWS = \
	$(OBJDIR)/CarlaBridgeFormat.cpp.o \
	$(OBJDIR)/CarlaBridgeFormatLV2.cpp-windows.o \
	$(OBJDIR)/CarlaBridgeToolkitNative.cpp-windows.o

$(BINDIR)/$(MODULENAME)-lv2-windows$(APP_EXT): $(OBJS_WINDOWS) $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-lv2-windows$(APP_EXT)"
	$(SILENT)$(CXX) $(OBJS_WINDOWS) $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_WINDOWS_FLAGS) -o $@

$(OBJDIR)/%.cpp-windows.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (Windows)"
	$(SILENT)$(CXX) $< $(BUILD_WINDOWS_FLAGS) -c -o $@

-include $(OBJS_WINDOWS:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
