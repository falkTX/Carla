#!/usr/bin/make -f
# Makefile for carla-bridges #
# -------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -DBUILD_BRIDGE
BUILD_CXX_FLAGS += -I. -I../backend -I../includes -I../utils -isystem ../modules
BUILD_CXX_FLAGS += $(LIBLO_FLAGS)

BUILD_LV2_FLAGS  = $(BUILD_CXX_FLAGS) -DBRIDGE_LV2
BUILD_VST_FLAGS  = $(BUILD_CXX_FLAGS) -DBRIDGE_VST

ifeq ($(CARLA_VESTIGE_HEADER),true)
BUILD_VST_FLAGS += -DVESTIGE_HEADER
endif

LINK_FLAGS += $(JUCE_CORE_LIBS)
LINK_FLAGS += $(LIBLO_LIBS)

# --------------------------------------------------------------

BUILD_LV2_GTK2_FLAGS    = $(BUILD_LV2_FLAGS) -DBRIDGE_GTK2 -DBRIDGE_LV2_GTK2 $(shell pkg-config --cflags gtk+-2.0)
LINK_LV2_GTK2_FLAGS     = $(LINK_FLAGS) $(shell pkg-config --libs gtk+-2.0) -ldl

BUILD_LV2_GTK3_FLAGS    = $(BUILD_LV2_FLAGS) -DBRIDGE_GTK3 -DBRIDGE_LV2_GTK3 $(shell pkg-config --cflags gtk+-3.0)
LINK_LV2_GTK3_FLAGS     = $(LINK_FLAGS) $(shell pkg-config --libs gtk+-3.0) -ldl

BUILD_LV2_QT4_FLAGS     = $(BUILD_LV2_FLAGS) -DBRIDGE_QT4 -DBRIDGE_LV2_QT4 $(shell pkg-config --cflags QtCore QtGui)
LINK_LV2_QT4_FLAGS      = $(LINK_FLAGS) $(shell pkg-config --libs QtCore QtGui) -ldl

BUILD_LV2_QT5_FLAGS     = $(BUILD_LV2_FLAGS) -DBRIDGE_QT5 -DBRIDGE_LV2_QT5 $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Widgets)
LINK_LV2_QT5_FLAGS      = $(LINK_FLAGS) $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Widgets) -ldl

BUILD_LV2_X11_FLAGS     = $(BUILD_LV2_FLAGS) -DBRIDGE_X11 -DBRIDGE_LV2_X11 $(X11_FLAGS)
LINK_LV2_X11_FLAGS      = $(LINK_FLAGS) $(X11_LIBS) -ldl

BUILD_LV2_COCOA_FLAGS   = $(BUILD_LV2_FLAGS) -DBRIDGE_COCOA -DBRIDGE_LV2_COCOA
LINK_LV2_COCOA_FLAGS    = $(LINK_FLAGS) -ldl

BUILD_LV2_WINDOWS_FLAGS = $(BUILD_LV2_FLAGS) -DBRIDGE_HWND -DBRIDGE_LV2_HWND
LINK_LV2_WINDOWS_FLAGS  = $(LINK_FLAGS) -static -mwindows

BUILD_VST_X11_FLAGS     = $(BUILD_VST_FLAGS) -DBRIDGE_X11 -DBRIDGE_VST_X11 $(X11_FLAGS)
LINK_VST_X11_FLAGS      = $(LINK_FLAGS) $(X11_LIBS) -ldl

# --------------------------------------------------------------

ifeq ($(HAVE_GTK2),true)
# TARGETS += ui_lv2-gtk2
endif

ifeq ($(HAVE_GTK3),true)
# TARGETS += ui_lv2-gtk3
endif

ifeq ($(HAVE_QT4),true)
# TARGETS += ui_lv2-qt4
endif

ifeq ($(HAVE_QT5),true)
# TARGETS += ui_lv2-qt5
endif

ifeq ($(HAVE_X11),true)
TARGETS += ui_lv2-x11
TARGETS += ui_vst-x11
endif

ifeq ($(MACOS),true)
TARGETS += ui_lv2-cocoa
endif

ifeq ($(WIN32),true)
TARGETS += ui_lv2-windows
endif

# --------------------------------------------------------------

all: $(TARGETS)

ui_lv2-gtk2:    ../../bin/carla-bridge-lv2-gtk2
ui_lv2-gtk3:    ../../bin/carla-bridge-lv2-gtk3
ui_lv2-qt4:     ../../bin/carla-bridge-lv2-qt4
ui_lv2-qt5:     ../../bin/carla-bridge-lv2-qt5
ui_lv2-x11:     ../../bin/carla-bridge-lv2-x11
ui_lv2-cocoa:   ../../bin/carla-bridge-lv2-cocoa
ui_lv2-windows: ../../bin/carla-bridge-lv2-windows.exe
ui_vst-x11:     ../../bin/carla-bridge-vst-x11

# --------------------------------------------------------------
# UI common

OBJS_LV2_LIBS = \
	../modules/juce_core.a \
	../modules/lilv.a

OBJS_VST_LIBS = \
	../modules/juce_core.a

%__lv2.o: %.cpp
	$(CXX) $< $(BUILD_LV2_FLAGS) -c -o $@

%__vst.o: %.cpp
	$(CXX) $< $(BUILD_VST_FLAGS) -c -o $@

resources.qt4.cpp: ../../resources/resources-theme.qrc
	$(RCC_QT4) $< -o $@

resources.qt5.cpp: ../../resources/resources-theme.qrc
	$(RCC_QT5) $< -o $@

CarlaBridgeToolkitQt4.moc: CarlaBridgeToolkitQt.cpp
	$(MOC_QT4) $< -DMOC_PARSING -o $@

CarlaBridgeToolkitQt5.moc: CarlaBridgeToolkitQt.cpp
	$(MOC_QT5) $< -DMOC_PARSING -o $@

# --------------------------------------------------------------
# ui_lv2-gtk2

OBJS_LV2_GTK2 = \
	CarlaBridgeClient__lv2.o \
	CarlaBridgeOsc__lv2.o \
	CarlaBridgeToolkitGtk__lv2-gtk2.o \
	CarlaBridgeUI-LV2__lv2-gtk2.o

../../bin/carla-bridge-lv2-gtk2: $(OBJS_LV2_GTK2) $(OBJS_LV2_LIBS)
	$(CXX) $^ $(LINK_LV2_GTK2_FLAGS) -o $@

%__lv2-gtk2.o: %.cpp
	$(CXX) $< $(BUILD_LV2_GTK2_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-gtk3

OBJS_LV2_GTK3 = \
	CarlaBridgeClient__lv2.o \
	CarlaBridgeOsc__lv2.o \
	CarlaBridgeToolkitGtk__lv2-gtk3.o \
	CarlaBridgeUI-LV2__lv2-gtk3.o

../../bin/carla-bridge-lv2-gtk3: $(OBJS_LV2_GTK3) $(OBJS_LV2_LIBS)
	$(CXX) $^ $(LINK_LV2_GTK3_FLAGS) -o $@

%__lv2-gtk3.o: %.cpp
	$(CXX) $< $(BUILD_LV2_GTK3_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-qt4

OBJS_LV2_QT4 = \
	CarlaBridgeClient__lv2.o \
	CarlaBridgeOsc__lv2.o \
	CarlaBridgeToolkitQt__lv2-qt4.o \
	CarlaBridgeUI-LV2__lv2-qt4.o

../../bin/carla-bridge-lv2-qt4: $(OBJS_LV2_QT4) $(OBJS_LV2_LIBS) ../modules/theme.qt4.a
	$(CXX) $^ $(LINK_LV2_QT4_FLAGS) -o $@

%__lv2-qt4.o: %.cpp CarlaBridgeToolkitQt4.moc resources.qt4.cpp
	$(CXX) $< $(BUILD_LV2_QT4_FLAGS) -I../modules/theme -c -o $@

# --------------------------------------------------------------
# ui_lv2-qt5

OBJS_LV2_QT5 = \
	CarlaBridgeClient__lv2.o \
	CarlaBridgeOsc__lv2.o \
	CarlaBridgeToolkitQt__lv2-qt5.o \
	CarlaBridgeUI-LV2__lv2-qt5.o

../../bin/carla-bridge-lv2-qt5: $(OBJS_LV2_QT5) $(OBJS_LV2_LIBS) ../modules/theme.qt5.a
	$(CXX) $^ $(LINK_LV2_QT5_FLAGS) -o $@

%__lv2-qt5.o: %.cpp CarlaBridgeToolkitQt5.moc resources.qt5.cpp
	$(CXX) $< $(BUILD_LV2_QT5_FLAGS) -I../modules/theme -c -o $@

# --------------------------------------------------------------
# ui_lv2-x11

OBJS_LV2_X11 = \
	CarlaBridgeClient__lv2.o \
	CarlaBridgeOsc__lv2.o \
	CarlaBridgeToolkitPlugin__lv2-x11.o \
	CarlaBridgeUI-LV2__lv2-x11.o

../../bin/carla-bridge-lv2-x11: $(OBJS_LV2_X11) $(OBJS_LV2_LIBS)
	$(CXX) $^ $(LINK_LV2_X11_FLAGS) -o $@

%__lv2-x11.o: %.cpp
	$(CXX) $< $(BUILD_LV2_X11_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-cocoa

OBJS_LV2_COCOA = \
	CarlaBridgeClient__lv2.o \
	CarlaBridgeOsc__lv2.o \
	CarlaBridgeToolkitPlugin__lv2-cocoa.o \
	CarlaBridgeUI-LV2__lv2-cocoa.o

../../bin/carla-bridge-lv2-cocoa: $(OBJS_LV2_COCOA) $(OBJS_LV2_LIBS)
	$(CXX) $^ $(LINK_LV2_COCOA_FLAGS) -o $@

%__lv2-cocoa.o: %.cpp
	$(CXX) $< $(BUILD_LV2_COCOA_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-windows

OBJS_LV2_WINDOWS = \
	CarlaBridgeClient__lv2.o \
	CarlaBridgeOsc__lv2.o \
	CarlaBridgeToolkitPlugin__lv2-windows.o \
	CarlaBridgeUI-LV2__lv2-windows.o

../../bin/carla-bridge-lv2-windows.exe: $(OBJS_LV2_WINDOWS) $(OBJS_LV2_LIBS)
	$(CXX) $^ $(LINK_LV2_WINDOWS_FLAGS) -o $@

%__lv2-windows.o: %.cpp
	$(CXX) $< $(BUILD_LV2_WINDOWS_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_vst-x11

OBJS_VST_X11 = \
	CarlaBridgeClient__vst.o \
	CarlaBridgeOsc__vst.o \
	CarlaBridgeToolkitPlugin__vst-x11.o \
	CarlaBridgeUI-VST__vst-x11.o \

../../bin/carla-bridge-vst-x11: $(OBJS_VST_X11) $(OBJS_VST_LIBS)
	$(CXX) $^ $(LINK_VST_X11_FLAGS) -o $@

%__vst-x11.o: %.cpp
	$(CXX) $< $(BUILD_VST_X11_FLAGS) -c -o $@

# --------------------------------------------------------------

clean:
	$(RM) *.moc *.o
	$(RM) resources.*.cpp
	$(RM) ../../bin/carla-bridge-lv2-* ../../bin/carla-bridge-vst-*

debug:
	$(MAKE) DEBUG=true

doxygen: carla_bridge.doxygen
	doxygen $<

# --------------------------------------------------------------
