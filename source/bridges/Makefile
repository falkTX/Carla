#!/usr/bin/make -f
# Makefile for carla-bridges #
# -------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

# TODO: remove in favour of juce gui stuff
# QT5_UI_FLAGS = $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Widgets)
# QT5_UI_LIBS  = $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Widgets)

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -DBUILD_BRIDGE
BUILD_CXX_FLAGS += -I. -I../backend -I../includes -I../utils -isystem ../modules -isystem ../modules/theme
BUILD_CXX_FLAGS += $(shell pkg-config --cflags liblo) $(QTCORE_FLAGS)
LINK_FLAGS      += $(shell pkg-config --libs liblo) $(QTCORE_LIBS)

# --------------------------------------------------------------
# Plugin bridges

BUILD_PLUGIN_FLAGS  = $(BUILD_CXX_FLAGS) $(QTXML_FLAGS) -DBUILD_BRIDGE_PLUGIN
BUILD_PLUGIN_FLAGS += -I../backend/engine -I../backend/plugin
LINK_PLUGIN_FLAGS   = $(LINK_FLAGS) $(QTXML_LIBS) -lpthread

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
BUILD_PLUGIN_FLAGS += -DWANT_LADSPA
# -DWANT_DSSI -DWANT_LV2 -DWANT_VST
# ifeq ($(CARLA_VESTIGE_HEADER),true)
# BUILD_PLUGIN_FLAGS += -DVESTIGE_HEADER
# endif
endif

LINK_PLUGIN_FLAGS  += $(JUCE_AUDIO_BASICS_LIBS)
LINK_PLUGIN_FLAGS  += $(JUCE_CORE_LIBS)
LINK_PLUGIN_FLAGS  += $(JUCE_DATA_STRUCTURES_LIBS)
LINK_PLUGIN_FLAGS  += $(JUCE_EVENTS_LIBS)
LINK_PLUGIN_FLAGS  += $(JUCE_GRAPHICS_LIBS)
LINK_PLUGIN_FLAGS  += $(JUCE_GUI_BASICS_LIBS)

# --------------------------------------------------------------
# Plugin bridges (POSIX)

POSIX_BUILD_FLAGS = $(BUILD_PLUGIN_FLAGS)
POSIX_32BIT_FLAGS = $(32BIT_FLAGS) -L/usr/lib32 -L/usr/lib/i386-linux-gnu
POSIX_64BIT_FLAGS = $(64BIT_FLAGS) -L/usr/lib64 -L/usr/lib/x86_64-linux-gnu
POSIX_LINK_FLAGS  = $(LINK_PLUGIN_FLAGS) -ldl

# --------------------------------------------------------------
# Plugin bridges (Windows)

WIN_BUILD_FLAGS = $(BUILD_PLUGIN_FLAGS) -DJACKBRIDGE_EXPORT
WIN_32BIT_FLAGS = $(32BIT_FLAGS)
WIN_64BIT_FLAGS = $(64BIT_FLAGS)
WIN_LINK_FLAGS  = $(LINK_PLUGIN_FLAGS) -L../modules

# --------------------------------------------------------------
# Plugin bridges (Native)

NATIVE_BUILD_FLAGS  = $(POSIX_BUILD_FLAGS)
# -DWANT_NATIVE
NATIVE_LINK_FLAGS   = $(POSIX_LINK_FLAGS)

# ifeq ($(HAVE_FLUIDSYNTH),true)
# NATIVE_BUILD_FLAGS += -DWANT_FLUIDSYNTH
# NATIVE_BUILD_FLAGS += $(shell pkg-config --cflags fluidsynth)
# NATIVE_LINK_FLAGS  += $(shell pkg-config --libs fluidsynth)
# endif
#
# ifeq ($(HAVE_LINUXSAMPLER),true)
# NATIVE_BUILD_FLAGS += -DWANT_LINUXSAMPLER
# NATIVE_BUILD_FLAGS += $(shell pkg-config --cflags linuxsampler)
# NATIVE_LINK_FLAGS  += $(shell pkg-config --libs linuxsampler)
# endif

# ifeq ($(HAVE_AF_DEPS),true)
# NATIVE_BUILD_FLAGS += -DWANT_AUDIOFILE
# NATIVE_LINK_FLAGS  += $(shell pkg-config --libs sndfile)
# ifeq ($(HAVE_FFMPEG),true)
# NATIVE_BUILD_FLAGS += -DHAVE_FFMPEG
# NATIVE_LINK_FLAGS  += $(shell pkg-config --libs libavcodec libavformat libavutil)
# endif
# endif
#
# ifeq ($(HAVE_MF_DEPS),true)
# NATIVE_BUILD_FLAGS += -DWANT_MIDIFILE
# NATIVE_LINK_FLAGS  += $(shell pkg-config --libs smf)
# endif
#
# ifeq ($(HAVE_OPENGL),true)
# NATIVE_BUILD_FLAGS += -DWANT_OPENGL
# NATIVE_LINK_FLAGS  += $(DGL_LIBS)
# endif

# ifeq ($(HAVE_ZYN_DEPS),true)
# NATIVE_BUILD_FLAGS += -DWANT_ZYNADDSUBFX
# NATIVE_LINK_FLAGS  += $(shell pkg-config --libs fftw3 mxml zlib)
# ifeq ($(HAVE_ZYN_UI_DEPS),true)
# NATIVE_LINK_FLAGS  += $(shell pkg-config --libs ntk_images ntk)
# endif
# endif

# --------------------------------------------------------------
# UI bridges

BUILD_UI_FLAGS = $(BUILD_CXX_FLAGS) -DBUILD_BRIDGE_UI
LINK_UI_FLAGS  = $(LINK_FLAGS) -ldl
LINK_UI_FLAGS += $(JUCE_CORE_LIBS)

BUILD_UI_LV2_FLAGS  = $(BUILD_UI_FLAGS) -DBRIDGE_LV2
BUILD_UI_VST_FLAGS  = $(BUILD_UI_FLAGS) -DBRIDGE_VST

ifeq ($(CARLA_VESTIGE_HEADER),true)
BUILD_UI_VST_FLAGS += -DVESTIGE_HEADER
endif

BUILD_UI_LV2_GTK2_FLAGS  = $(BUILD_UI_LV2_FLAGS) -DBRIDGE_GTK2 -DBRIDGE_LV2_GTK2 $(shell pkg-config --cflags gtk+-2.0 Qt5Core)
LINK_UI_LV2_GTK2_FLAGS   = $(LINK_UI_FLAGS) $(shell pkg-config --libs gtk+-2.0 Qt5Core)

BUILD_UI_LV2_GTK3_FLAGS  = $(BUILD_UI_LV2_FLAGS) -DBRIDGE_GTK3 -DBRIDGE_LV2_GTK3 $(shell pkg-config --cflags gtk+-3.0 Qt5Core)
LINK_UI_LV2_GTK3_FLAGS   = $(LINK_UI_FLAGS) $(shell pkg-config --libs gtk+-3.0 Qt5Core)

BUILD_UI_LV2_QT4_FLAGS   = $(BUILD_UI_LV2_FLAGS) -DBRIDGE_QT4 -DBRIDGE_LV2_QT4 $(shell pkg-config --cflags QtCore QtGui)
LINK_UI_LV2_QT4_FLAGS    = $(LINK_UI_FLAGS) $(shell pkg-config --libs QtCore QtGui)

BUILD_UI_LV2_QT5_FLAGS   = $(BUILD_UI_LV2_FLAGS) -DBRIDGE_QT5 -DBRIDGE_LV2_QT5 $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Widgets)
LINK_UI_LV2_QT5_FLAGS    = $(LINK_UI_FLAGS) $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Widgets)

BUILD_UI_LV2_COCOA_FLAGS = $(BUILD_UI_LV2_FLAGS) -DBRIDGE_COCOA -DBRIDGE_LV2_COCOA $(QT5_UI_FLAGS)
LINK_UI_LV2_COCOA_FLAGS  = $(LINK_UI_FLAGS) $(QT5_UI_LIBS)

BUILD_UI_LV2_WIN32_FLAGS = $(BUILD_UI_LV2_FLAGS) -DBRIDGE_HWND -DBRIDGE_LV2_WIN32 $(QT5_UI_FLAGS)
LINK_UI_LV2_WIN32_FLAGS  = $(LINK_FLAGS) $(QT5_UI_LIBS) $(EXTRA_LIBS) -mwindows -lwinspool -lole32 -luuid -limm32 -lshell32 -lws2_32 -static

BUILD_UI_LV2_X11_FLAGS   = $(BUILD_UI_LV2_FLAGS) -DBRIDGE_X11 -DBRIDGE_LV2_X11 $(QT5_UI_FLAGS)
LINK_UI_LV2_X11_FLAGS    = $(LINK_UI_FLAGS) $(QT5_UI_LIBS)

BUILD_UI_VST_MAC_FLAGS   = $(BUILD_UI_VST_FLAGS) -DBRIDGE_MAC -DBRIDGE_VST_MAC $(QT5_UI_FLAGS)
LINK_UI_VST_MAC_FLAGS    = $(LINK_UI_FLAGS) $(QT5_UI_LIBS)

BUILD_UI_VST_HWND_FLAGS  = $(BUILD_UI_VST_FLAGS) -DBRIDGE_HWND -DBRIDGE_VST_HWND $(QT5_UI_FLAGS)
LINK_UI_VST_HWND_FLAGS   = $(LINK_FLAGS) $(QT5_UI_LIBS) $(EXTRA_LIBS) -mwindows -lwinspool -lole32 -luuid -limm32 -lshell32 -lws2_32 -static

BUILD_UI_VST_X11_FLAGS   = $(BUILD_UI_VST_FLAGS) -DBRIDGE_X11 -DBRIDGE_VST_X11 $(QT5_UI_FLAGS)
LINK_UI_VST_X11_FLAGS    = $(LINK_UI_FLAGS) $(QT5_UI_LIBS)

# --------------------------------------------------------------

TARGETS  = native

# ifeq ($(CARLA_PLUGIN_SUPPORT),true)
# ifeq ($(HAVE_GTK2),true)
# TARGETS += ui_lv2-gtk2
# endif
# ifeq ($(HAVE_GTK3),true)
# TARGETS += ui_lv2-gtk3
# endif
# ifeq ($(HAVE_QT4),true)
# TARGETS += ui_lv2-qt4
# else
# ifeq ($(HAVE_QT5),true)
# TARGETS += ui_lv2-qt5
# endif
# endif
# TARGETS += ui_lv2-x11
# TARGETS += ui_vst-x11
# endif

ifeq ($(WIN32),true)
TARGETS  = carla-bridge-win32.exe
TARGETS += carla-bridge-win64.exe
TARGETS += carla-bridge-lv2-windows.exe
TARGETS += carla-bridge-vst-hwnd.exe
endif

# --------------------------------------------------------------

all: $(TARGETS)

ui_lv2-gtk2: carla-bridge-lv2-gtk2
ui_lv2-gtk3: carla-bridge-lv2-gtk3
ui_lv2-qt4:  carla-bridge-lv2-qt4
ui_lv2-qt5:  carla-bridge-lv2-qt5

ui_lv2-cocoa: carla-bridge-lv2-cocoa
ui_lv2-win32: carla-bridge-lv2-windows.exe
ui_lv2-x11:   carla-bridge-lv2-x11

ui_vst-mac:  carla-bridge-vst-mac
ui_vst-hwnd: carla-bridge-vst-hwnd.exe
ui_vst-x11:  carla-bridge-vst-x11

# --------------------------------------------------------------

native:  carla-bridge-native
posix32: carla-bridge-posix32
posix64: carla-bridge-posix64
win32:   carla-bridge-win32.exe
win64:   carla-bridge-win64.exe

# --------------------------------------------------------------
# UI common

OBJS_UI_LV2_LIBS = \
	../modules/lilv.a \
	../modules/juce_core.a

OBJS_UI_VST_LIBS = \
	../modules/juce_core.a

%__lv2.o: %.cpp
	$(CXX) $< $(BUILD_UI_LV2_FLAGS) -c -o $@

%__vst.o: %.cpp
	$(CXX) $< $(BUILD_UI_VST_FLAGS) -c -o $@

resources.cpp: ../../resources/resources-theme.qrc
	$(RCC) $< -o $@

resources.qt4.cpp: ../../resources/resources-theme.qrc
	$(RCC_QT4) $< -o $@

CarlaBridgeToolkitQt.moc: CarlaBridgeToolkitQt.cpp
	$(MOC) $< -DMOC_PARSING -o $@

CarlaBridgeToolkitQt4.moc: CarlaBridgeToolkitQt.cpp
	$(MOC_QT4) $< -DMOC_PARSING -o $@

# --------------------------------------------------------------
# ui_lv2-gtk2

OBJS_UI_LV2_GTK2 = CarlaBridgeUI-LV2__lv2-gtk2.o \
	CarlaBridgeClient__lv2.o CarlaBridgeOsc__lv2.o \
	CarlaBridgeToolkit__lv2.o CarlaBridgeToolkitGtk__lv2-gtk2.o

carla-bridge-lv2-gtk2: $(OBJS_UI_LV2_GTK2) $(OBJS_UI_LV2_LIBS)
	$(CXX) $^ $(LINK_UI_LV2_GTK2_FLAGS) -o $@

%__lv2-gtk2.o: %.cpp
	$(CXX) $< $(BUILD_UI_LV2_GTK2_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-gtk3

OBJS_UI_LV2_GTK3 = CarlaBridgeUI-LV2__lv2-gtk3.o \
	CarlaBridgeClient__lv2.o CarlaBridgeOsc__lv2.o \
	CarlaBridgeToolkit__lv2.o CarlaBridgeToolkitGtk__lv2-gtk3.o

carla-bridge-lv2-gtk3: $(OBJS_UI_LV2_GTK3) $(OBJS_UI_LV2_LIBS)
	$(CXX) $^ $(LINK_UI_LV2_GTK3_FLAGS) -o $@

%__lv2-gtk3.o: %.cpp
	$(CXX) $< $(BUILD_UI_LV2_GTK3_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-qt4

OBJS_UI_LV2_QT4 = CarlaBridgeUI-LV2__lv2-qt4.o \
	CarlaBridgeClient__lv2.o CarlaBridgeOsc__lv2.o \
	CarlaBridgeToolkit__lv2.o CarlaBridgeToolkitQt__lv2-qt4.o

carla-bridge-lv2-qt4: $(OBJS_UI_LV2_QT4) $(OBJS_UI_LV2_LIBS) ../modules/theme.qt4.a
	$(CXX) $^ $(LINK_UI_LV2_QT4_FLAGS) -o $@

%__lv2-qt4.o: %.cpp CarlaBridgeToolkitQt4.moc resources.qt4.cpp
	$(CXX) $< $(BUILD_UI_LV2_QT4_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-qt5

OBJS_UI_LV2_QT5 = CarlaBridgeUI-LV2__lv2-qt5.o \
	CarlaBridgeClient__lv2.o CarlaBridgeOsc__lv2.o \
	CarlaBridgeToolkit__lv2.o CarlaBridgeToolkitQt__lv2-qt5.o

carla-bridge-lv2-qt5: $(OBJS_UI_LV2_QT5) $(OBJS_UI_LV2_LIBS) ../modules/theme.a
	$(CXX) $^ $(LINK_UI_LV2_QT5_FLAGS) -o $@

%__lv2-qt5.o: %.cpp CarlaBridgeToolkitQt.moc resources.cpp
	$(CXX) $< $(BUILD_UI_LV2_QT5_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-cocoa

OBJS_UI_LV2_COCOA = CarlaBridgeUI-LV2__lv2-cocoa.o \
	CarlaBridgeClient__lv2.o CarlaBridgeOsc__lv2.o \
	CarlaBridgeToolkit__lv2.o CarlaBridgeToolkitQt__lv2-cocoa.o

carla-bridge-lv2-cocoa: $(OBJS_UI_LV2_COCOA) $(OBJS_UI_LV2_LIBS) ../modules/theme.a
	$(CXX) $^ $(LINK_UI_LV2_COCOA_FLAGS) -o $@

%__lv2-cocoa.o: %.cpp CarlaBridgeToolkitQt.moc resources.cpp
	$(CXX) $< $(BUILD_UI_LV2_COCOA_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-windows

OBJS_UI_LV2_WIN32 = CarlaBridgeUI-LV2__lv2-windows.o \
	CarlaBridgeClient__lv2.o CarlaBridgeOsc__lv2.o \
	CarlaBridgeToolkit__lv2.o CarlaBridgeToolkitQt__lv2-windows.o

carla-bridge-lv2-windows.exe: $(OBJS_UI_LV2_WIN32) $(OBJS_UI_LV2_LIBS) ../modules/theme.a
	$(CXX) $^ $(LINK_UI_LV2_WIN32_FLAGS) -o $@

%__lv2-windows.o: %.cpp CarlaBridgeToolkitQt.moc resources.cpp
	$(CXX) $< $(BUILD_UI_LV2_WIN32_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-x11

OBJS_UI_LV2_X11 = CarlaBridgeUI-LV2__lv2-x11.o \
	CarlaBridgeClient__lv2.o CarlaBridgeOsc__lv2.o \
	CarlaBridgeToolkit__lv2.o CarlaBridgeToolkitQt__lv2-x11.o

carla-bridge-lv2-x11: $(OBJS_UI_LV2_X11) $(OBJS_UI_LV2_LIBS) ../modules/theme.a
	$(CXX) $^ $(LINK_UI_LV2_X11_FLAGS) -o $@

%__lv2-x11.o: %.cpp CarlaBridgeToolkitQt.moc resources.cpp
	$(CXX) $< $(BUILD_UI_LV2_X11_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_vst-mac

OBJS_UI_VST_MAC = CarlaBridgeUI-VST__vst-mac.o \
	CarlaBridgeClient__vst.o CarlaBridgeOsc__vst.o \
	CarlaBridgeToolkit__vst.o CarlaBridgeToolkitQt__vst-mac.o

carla-bridge-vst-mac: $(OBJS_UI_VST_MAC) $(OBJS_UI_VST_LIBS) ../modules/theme.a
	$(CXX) $^ $(LINK_UI_VST_MAC_FLAGS) -o $@

%__vst-mac.o: %.cpp CarlaBridgeToolkitQt.moc resources.cpp
	$(CXX) $< $(BUILD_UI_VST_MAC_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_vst-hwnd

OBJS_UI_VST_HWND = CarlaBridgeUI-VST__vst-hwnd.o \
	CarlaBridgeClient__vst.o CarlaBridgeOsc__vst.o \
	CarlaBridgeToolkit__vst.o CarlaBridgeToolkitQt__vst-hwnd.o

carla-bridge-vst-hwnd.exe: $(OBJS_UI_VST_HWND) $(OBJS_UI_VST_LIBS) ../modules/theme.a
	$(CXX) $^ $(LINK_UI_VST_HWND_FLAGS) -o $@

%__vst-hwnd.o: %.cpp CarlaBridgeToolkitQt.moc resources.cpp
	$(CXX) $< $(BUILD_UI_VST_HWND_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_vst-x11

OBJS_UI_VST_X11 = CarlaBridgeUI-VST__vst-x11.o \
	CarlaBridgeClient__vst.o CarlaBridgeOsc__vst.o \
	CarlaBridgeToolkit__vst.o CarlaBridgeToolkitQt__vst-x11.o

carla-bridge-vst-x11: $(OBJS_UI_VST_X11) $(OBJS_UI_VST_LIBS) ../modules/theme.a
	$(CXX) $^ $(LINK_UI_VST_X11_FLAGS) -o $@

%__vst-x11.o: %.cpp CarlaBridgeToolkitQt.moc resources.cpp
	$(CXX) $< $(BUILD_UI_VST_X11_FLAGS) -c -o $@

# --------------------------------------------------------------
# native

OBJS_NATIVE  = CarlaBridgePlugin__native.o \
	CarlaBridgeClient__native.o CarlaBridgeOsc__native.o

# carla-engine
OBJS_NATIVE += \
	../backend/engine/CarlaEngine__native.o \
	../backend/engine/CarlaEngineOsc__native.o \
	../backend/engine/CarlaEngineThread__native.o \
	../backend/engine/CarlaEngineJack__native.o \
	../backend/engine/CarlaEngineBridge__native.o

# carla-plugin
OBJS_NATIVE += \
	../backend/plugin/CarlaPlugin__native.o \
	../backend/plugin/CarlaPluginThread__native.o \
	../backend/plugin/NativePlugin__native.o \
	../backend/plugin/LadspaPlugin__native.o \
	../backend/plugin/DssiPlugin__native.o \
	../backend/plugin/Lv2Plugin__native.o \
	../backend/plugin/VstPlugin__native.o \
	../backend/plugin/AuPlugin__native.o \
	../backend/plugin/JucePlugin__native.o \
	../backend/plugin/FluidSynthPlugin__native.o \
	../backend/plugin/LinuxSamplerPlugin__native.o

# carla-standalone
OBJS_NATIVE += \
	../backend/standalone/CarlaStandalone__native.o

# libs
OBJS_NATIVE += \
	../modules/jackbridge.a \
	../modules/juce_audio_basics.a \
	../modules/juce_core.a \
	../modules/juce_events.a \
	../modules/rtmempool.a

# 	../modules/carla_native.a \
# 	../modules/juce_data_structures.a \
# 	../modules/juce_graphics.a \
# 	../modules/juce_gui_basics.a \

# ifeq ($(HAVE_OPENGL),true)
# OBJS_NATIVE += \
# 	../modules/dgl.a
# endif

# ifeq ($(CARLA_PLUGIN_SUPPORT),true)
# OBJS_NATIVE += \
# 	../modules/lilv.a
# endif

carla-bridge-native: $(FILES) $(OBJS_NATIVE)
	$(CXX) $(OBJS_NATIVE) $(NATIVE_LINK_FLAGS) -o $@

%__native.o: %.cpp
	$(CXX) $< $(NATIVE_BUILD_FLAGS) -c -o $@

# --------------------------------------------------------------
# posix32

OBJS_POSIX32  = CarlaBridgePlugin__posix32.o \
	CarlaBridgeClient__posix32.o CarlaBridgeOsc__posix32.o

# carla-engine
OBJS_POSIX32 += \
	../backend/engine/CarlaEngine__posix32.o \
	../backend/engine/CarlaEngineOsc__posix32.o \
	../backend/engine/CarlaEngineThread__posix32.o \
	../backend/engine/CarlaEngineJack__posix32.o \
	../backend/engine/CarlaEngineBridge__posix32.o

# carla-plugin
OBJS_POSIX32 += \
	../backend/plugin/CarlaPlugin__posix32.o \
	../backend/plugin/CarlaPluginThread__posix32.o \
	../backend/plugin/NativePlugin__posix32.o \
	../backend/plugin/LadspaPlugin__posix32.o \
	../backend/plugin/DssiPlugin__posix32.o \
	../backend/plugin/Lv2Plugin__posix32.o \
	../backend/plugin/VstPlugin__posix32.o \
	../backend/plugin/Vst3Plugin__posix32.o \
	../backend/plugin/FluidSynthPlugin__posix32.o \
	../backend/plugin/LinuxSamplerPlugin__posix32.o

# carla-standalone
OBJS_POSIX32 += \
	../backend/standalone/CarlaStandalone__posix32.o

# libs
OBJS_POSIX32 += \
	../modules/lilv.posix32.a \
	../modules/rtmempool.posix32.a

carla-bridge-posix32: $(FILES) $(OBJS_POSIX32)
	$(CXX) $(OBJS_POSIX32) $(POSIX_LINK_FLAGS) $(POSIX_32BIT_FLAGS) -o $@

%__posix32.o: %.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_32BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
# posix64

OBJS_POSIX64  = CarlaBridgePlugin__posix64.o \
	CarlaBridgeClient__posix64.o CarlaBridgeOsc__posix64.o

# carla-engine
OBJS_POSIX64 += \
	../backend/engine/CarlaEngine__posix64.o \
	../backend/engine/CarlaEngineOsc__posix64.o \
	../backend/engine/CarlaEngineThread__posix64.o \
	../backend/engine/CarlaEngineJack__posix64.o \
	../backend/engine/CarlaEngineBridge__posix64.o

# carla-plugin
OBJS_POSIX64 += \
	../backend/plugin/CarlaPlugin__posix64.o \
	../backend/plugin/CarlaPluginThread__posix64.o \
	../backend/plugin/NativePlugin__posix64.o \
	../backend/plugin/LadspaPlugin__posix64.o \
	../backend/plugin/DssiPlugin__posix64.o \
	../backend/plugin/Lv2Plugin__posix64.o \
	../backend/plugin/VstPlugin__posix64.o \
	../backend/plugin/Vst3Plugin__posix64.o \
	../backend/plugin/FluidSynthPlugin__posix64.o \
	../backend/plugin/LinuxSamplerPlugin__posix64.o

# carla-standalone
OBJS_POSIX64 += \
	../backend/standalone/CarlaStandalone__posix64.o

# libs
OBJS_POSIX64 += \
	../modules/lilv.posix64.a \
	../modules/rtmempool.posix64.a

carla-bridge-posix64: $(FILES) $(OBJS_POSIX64)
	$(CXX) $(OBJS_POSIX64) $(POSIX_LINK_FLAGS) $(POSIX_64BIT_FLAGS) -o $@

%__posix64.o: %.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
# win32

OBJS_WIN32  = CarlaBridgePlugin__win32.o \
	CarlaBridgeClient__win32.o CarlaBridgeOsc__win32.o

# carla-engine
OBJS_WIN32 += \
	../backend/engine/CarlaEngine__win32.o \
	../backend/engine/CarlaEngineOsc__win32.o \
	../backend/engine/CarlaEngineThread__win32.o \
	../backend/engine/CarlaEngineBridge__win32.o \
	../backend/engine/CarlaEngineJack__win32.o

# carla-plugin
OBJS_WIN32 += \
	../backend/plugin/CarlaPlugin__win32.o \
	../backend/plugin/CarlaPluginThread__win32.o \
	../backend/plugin/NativePlugin__win32.o \
	../backend/plugin/LadspaPlugin__win32.o \
	../backend/plugin/DssiPlugin__win32.o \
	../backend/plugin/Lv2Plugin__win32.o \
	../backend/plugin/VstPlugin__win32.o \
	../backend/plugin/Vst3Plugin__win32.o \
	../backend/plugin/FluidSynthPlugin__win32.o \
	../backend/plugin/LinuxSamplerPlugin__win32.o

# carla-standalone
OBJS_WIN32 += \
	../backend/standalone/CarlaStandalone__win32.o

# libs
OBJS_WIN32 += \
	../modules/juce_core.win32.a \
	../modules/lilv.win32.a \
	../modules/rtmempool.win32.a

carla-bridge-win32.exe: $(FILES) $(OBJS_WIN32) ../modules/jackbridge-win32.dll
	$(CXX) $(OBJS_WIN32) $(WIN_32BIT_FLAGS) $(WIN_LINK_FLAGS) -ljackbridge-win32 -o $@

%__win32.o: %.cpp
	$(CXX) $< $(WIN_BUILD_FLAGS) $(WIN_32BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
# win64

OBJS_WIN64  = CarlaBridgePlugin__win64.o \
	CarlaBridgeClient__win64.o CarlaBridgeOsc__win64.o

# carla-engine
OBJS_WIN64 += \
	../backend/engine/CarlaEngine__win64.o \
	../backend/engine/CarlaEngineOsc__win64.o \
	../backend/engine/CarlaEngineThread__win64.o \
	../backend/engine/CarlaEngineBridge__win64.o \
	../backend/engine/CarlaEngineJack__win64.o

# carla-plugin
OBJS_WIN64 += \
	../backend/plugin/CarlaPlugin__win64.o \
	../backend/plugin/CarlaPluginThread__win64.o \
	../backend/plugin/NativePlugin__win64.o \
	../backend/plugin/LadspaPlugin__win64.o \
	../backend/plugin/DssiPlugin__win64.o \
	../backend/plugin/Lv2Plugin__win64.o \
	../backend/plugin/VstPlugin__win64.o \
	../backend/plugin/Vst3Plugin__win64.o \
	../backend/plugin/FluidSynthPlugin__win64.o \
	../backend/plugin/LinuxSamplerPlugin__win64.o

# carla-standalone
OBJS_WIN64 += \
	../backend/standalone/CarlaStandalone__win64.o

# libs
OBJS_WIN64 += \
	../modules/lilv.win64.a \
	../modules/rtmempool.win64.a

carla-bridge-win64.exe: $(FILES) $(OBJS_WIN64) ../modules/jackbridge-win64.dll
	$(CXX) $(OBJS_WIN64) $(WIN_64BIT_FLAGS) $(WIN_LINK_FLAGS) -ljackbridge-win64 -o $@

%__win64.o: %.cpp
	$(CXX) $< $(WIN_BUILD_FLAGS) $(WIN_64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

.FORCE:
.PHONY: .FORCE

../modules/juce_core.%.a: .FORCE
	$(MAKE) -C ../modules juce_core_$*

../modules/lilv.%.a: .FORCE
	$(MAKE) -C ../modules lilv_$*

../modules/rtmempool.%.a: .FORCE
	$(MAKE) -C ../modules rtmempool_$*

../modules/%.a: .FORCE
	$(MAKE) -C ../modules $*

../modules/%.dll:
	$(MAKE) -C ../modules $*

# --------------------------------------------------------------

doxygen: carla_bridge.doxygen
	doxygen $<

# --------------------------------------------------------------

clean:
	rm -f *.moc *.o *.dll *.dylib *.so *.exe
	rm -f resources.cpp resources.qt4.cpp
	rm -f $(OBJS_NATIVE)
	rm -f $(OBJS_POSIX32)
	rm -f $(OBJS_POSIX64)
	rm -f $(OBJS_WIN32)
	rm -f $(OBJS_WIN64)
	rm -f carla-bridge-lv2-gtk2 carla-bridge-lv2-gtk3 carla-bridge-lv2-qt4 carla-bridge-lv2-qt5 carla-bridge-lv2-cocoa carla-bridge-lv2-x11
	rm -f carla-bridge-vst-mac carla-bridge-vst-x11
	rm -f carla-bridge-native carla-bridge-posix32 carla-bridge-posix64

# --------------------------------------------------------------

debug:
	$(MAKE) DEBUG=true
