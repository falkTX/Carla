#!/usr/bin/make -f
# Makefile for carla-bridges #
# ---------------------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

HAVE_GTK2        = $(shell pkg-config --exists gtk+-2.0 && echo true)
HAVE_GTK3        = $(shell pkg-config --exists gtk+-3.0 && echo true)

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -DBUILD_BRIDGE -pthread
BUILD_CXX_FLAGS += -I. -I../backend -I../includes -I../utils
BUILD_CXX_FLAGS += $(shell pkg-config --cflags liblo QtCore QtXml)
LINK_FLAGS      += $(shell pkg-config --libs liblo QtCore QtXml) -lpthread

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
BUILD_CXX_FLAGS += -DWANT_LADSPA -DWANT_DSSI -DWANT_LV2 -DWANT_VST
endif

ifeq ($(HAVE_QT5),true)
QT_UI_FLAGS      = $(shell pkg-config --cflags QtWidgets)
QT_UI_LIBS       = $(shell pkg-config --libs QtWidgets)
else
QT_UI_FLAGS      = $(shell pkg-config --cflags QtGui)
QT_UI_LIBS       = $(shell pkg-config --libs QtGui)
endif

# --------------------------------------------------------------
# Plugin bridges

BUILD_PLUGIN_FLAGS  = $(BUILD_CXX_FLAGS)
BUILD_PLUGIN_FLAGS += -DBUILD_BRIDGE_PLUGIN -DBRIDGE_PLUGIN -DWANT_JACK
BUILD_PLUGIN_FLAGS += -I../backend/engine -I../backend/plugin -I../libs -I../libs/distrho
BUILD_PLUGIN_FLAGS += $(shell pkg-config --cflags gl)

LINK_PLUGIN_FLAGS   = $(LINK_FLAGS)
LINK_PLUGIN_FLAGS  += $(shell pkg-config --libs gl)

NATIVE_BUILD_FLAGS  = $(BUILD_PLUGIN_FLAGS) $(shell pkg-config --cflags jack) -DWANT_NATIVE
NATIVE_LINK_FLAGS   = $(LINK_PLUGIN_FLAGS) $(shell pkg-config --libs jack) -ldl

ifeq ($(HAVE_AF_DEPS),true)
NATIVE_LINK_FLAGS  += $(shell pkg-config --libs libavcodec libavformat sndfile)
endif

ifeq ($(HAVE_ZYN_DEPS),true)
NATIVE_LINK_FLAGS  += $(shell pkg-config --libs fftw3 mxml)
endif

ifeq ($(HAVE_SUIL),true)
NATIVE_BUILD_FLAGS += $(shell pkg-config --cflags suil-0) -DWANT_SUIL
NATIVE_LINK_FLAGS  += $(shell pkg-config --libs suil-0)
endif

POSIX_BUILD_FLAGS   = $(BUILD_PLUGIN_FLAGS)
POSIX_32BIT_FLAGS   = $(32BIT_FLAGS) -L/usr/lib32 -L/usr/lib/i386-linux-gnu
POSIX_64BIT_FLAGS   = $(64BIT_FLAGS) -L/usr/lib64 -L/usr/lib/x86_64-linux-gnu
POSIX_LINK_FLAGS    = $(LINK_PLUGIN_FLAGS) $(shell pkg-config --libs jack) -ldl

WIN_BUILD_FLAGS     = $(BUILD_PLUGIN_FLAGS) -DJACKBRIDGE_EXPORT -DPTW32_STATIC_LIB -I../libs/jackbridge
WIN_32BIT_FLAGS     = $(32BIT_FLAGS)
WIN_64BIT_FLAGS     = $(64BIT_FLAGS)
WIN_LINK_FLAGS      = $(LINK_PLUGIN_FLAGS) -mwindows -L../libs -lpthread -lwinspool -lole32 -luuid -limm32 -lshell32 -lws2_32

# --------------------------------------------------------------
# UI bridges

BUILD_UI_FLAGS = $(BUILD_CXX_FLAGS) -DBUILD_BRIDGE_UI
LINK_UI_FLAGS  = $(LINK_FLAGS) -ldl

BUILD_UI_LV2_FLAGS = $(BUILD_UI_FLAGS) -DBRIDGE_LV2
BUILD_UI_VST_FLAGS = $(BUILD_UI_FLAGS) -DBRIDGE_VST

BUILD_UI_LV2_GTK2_FLAGS = $(BUILD_UI_LV2_FLAGS) -DBRIDGE_GTK2 -DBRIDGE_LV2_GTK2 $(shell pkg-config --cflags gtk+-2.0)
LINK_UI_LV2_GTK2_FLAGS  = $(LINK_UI_FLAGS) $(shell pkg-config --libs gtk+-2.0)

BUILD_UI_LV2_GTK3_FLAGS = $(BUILD_UI_LV2_FLAGS) -DBRIDGE_GTK3 -DBRIDGE_LV2_GTK3 $(shell pkg-config --cflags gtk+-3.0)
LINK_UI_LV2_GTK3_FLAGS  = $(LINK_UI_FLAGS) $(shell pkg-config --libs gtk+-3.0)

BUILD_UI_LV2_QT4_FLAGS  = $(BUILD_UI_LV2_FLAGS) -DBRIDGE_QT4 -DBRIDGE_LV2_QT4 $(shell pkg-config --cflags QtGui)
LINK_UI_LV2_QT4_FLAGS   = $(LINK_UI_FLAGS) $(shell pkg-config --libs QtGui)

BUILD_UI_LV2_QT5_FLAGS  = $(BUILD_UI_LV2_FLAGS) -DBRIDGE_QT5 -DBRIDGE_LV2_QT5 $(shell pkg-config --cflags QtWidgets)
LINK_UI_LV2_QT5_FLAGS   = $(LINK_UI_FLAGS) $(shell pkg-config --libs QtWidgets)

BUILD_UI_LV2_HWND_FLAGS = $(BUILD_UI_VST_FLAGS) -DBRIDGE_HWND -DBRIDGE_LV2_HWND $(QT_UI_FLAGS)
LINK_UI_LV2_HWND_FLAGS  = $(LINK_FLAGS) $(QT_UI_LIBS) -mwindows -static

BUILD_UI_LV2_X11_FLAGS  = $(BUILD_UI_LV2_FLAGS) -DBRIDGE_X11 -DBRIDGE_LV2_X11 $(QT_UI_FLAGS)
LINK_UI_LV2_X11_FLAGS   = $(LINK_UI_FLAGS) $(QT_UI_LIBS)

BUILD_UI_VST_HWND_FLAGS = $(BUILD_UI_VST_FLAGS) -DBRIDGE_HWND -DBRIDGE_VST_HWND $(QT_UI_FLAGS)
LINK_UI_VST_HWND_FLAGS  = $(LINK_FLAGS) $(QT_UI_LIBS) -mwindows -static

BUILD_UI_VST_X11_FLAGS  = $(BUILD_UI_VST_FLAGS) -DBRIDGE_X11 -DBRIDGE_VST_X11 $(QT_UI_FLAGS)
LINK_UI_VST_X11_FLAGS   = $(LINK_UI_FLAGS) $(QT_UI_LIBS)

# --------------------------------------------------------------

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
all: native ui_lv2-gtk2 ui_lv2-gtk3 ui_lv2-qt4 ui_lv2-qt5 ui_lv2-x11 ui_vst-x11

ifeq ($(HAVE_GTK2),true)
ui_lv2-gtk2: carla-bridge-lv2-gtk2
else
ui_lv2-gtk2:
endif

ifeq ($(HAVE_GTK3),true)
ui_lv2-gtk3: carla-bridge-lv2-gtk3
else
ui_lv2-gtk3:
endif

ifneq ($(HAVE_QT5),true)
ui_lv2-qt4: carla-bridge-lv2-qt4
ui_lv2-qt5:
else
ui_lv2-qt4:
ui_lv2-qt5: carla-bridge-lv2-qt5
endif

ui_lv2-hwnd: carla-bridge-lv2-hwnd.exe
ui_lv2-x11:  carla-bridge-lv2-x11

ui_vst-hwnd: carla-bridge-vst-hwnd.exe
ui_vst-x11:  carla-bridge-vst-x11

else # CARLA_PLUGIN_SUPPORT
all:
endif

# --------------------------------------------------------------

native:  carla-bridge-native
posix32: carla-bridge-posix32
posix64: carla-bridge-posix64
win32:   carla-bridge-win32.exe
win64:   carla-bridge-win64.exe

# --------------------------------------------------------------
# UI common

%__lv2.o: %.cpp
	$(CXX) $< $(BUILD_UI_LV2_FLAGS) -c -o $@

%__vst.o: %.cpp
	$(CXX) $< $(BUILD_UI_VST_FLAGS) -c -o $@

OBJS_UI_LV2_LIBS = \
	../libs/lilv.a \
	../libs/rtmempool.a

# --------------------------------------------------------------
# ui_lv2-gtk2

OBJS_UI_LV2_GTK2 = CarlaBridgeUI-LV2__lv2-gtk2.o \
	CarlaBridgeClient__lv2.o CarlaBridgeOsc__lv2.o \
	CarlaBridgeToolkit__lv2.o CarlaBridgeToolkitGtk__lv2-gtk2.o

carla-bridge-lv2-gtk2: $(OBJS_UI_LV2_GTK2) $(OBJS_UI_LV2_LIBS)
	$(CXX) $^ $(LINK_UI_LV2_GTK2_FLAGS) -o $@ && $(STRIP) $@

%__lv2-gtk2.o: %.cpp
	$(CXX) $< $(BUILD_UI_LV2_GTK2_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-gtk3

OBJS_UI_LV2_GTK3 = CarlaBridgeUI-LV2__lv2-gtk3.o \
	CarlaBridgeClient__lv2.o CarlaBridgeOsc__lv2.o \
	CarlaBridgeToolkit__lv2.o CarlaBridgeToolkitGtk__lv2-gtk3.o

carla-bridge-lv2-gtk3: $(OBJS_UI_LV2_GTK3) $(OBJS_UI_LV2_LIBS)
	$(CXX) $^ $(LINK_UI_LV2_GTK3_FLAGS) -o $@ && $(STRIP) $@

%__lv2-gtk3.o: %.cpp
	$(CXX) $< $(BUILD_UI_LV2_GTK3_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-qt4

OBJS_UI_LV2_QT4 = CarlaBridgeUI-LV2__lv2-qt4.o \
	CarlaBridgeClient__lv2.o CarlaBridgeOsc__lv2.o \
	CarlaBridgeToolkit__lv2.o CarlaBridgeToolkitQt__lv2-qt4.o

carla-bridge-lv2-qt4: $(OBJS_UI_LV2_QT4) $(OBJS_UI_LV2_LIBS)
	$(CXX) $^ $(LINK_UI_LV2_QT4_FLAGS) -o $@ && $(STRIP) $@

%__lv2-qt4.o: %.cpp
	$(CXX) $< $(BUILD_UI_LV2_QT4_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-qt5

OBJS_UI_LV2_QT5 = CarlaBridgeUI-LV2__lv2-qt5.o \
	CarlaBridgeClient__lv2.o CarlaBridgeOsc__lv2.o \
	CarlaBridgeToolkit__lv2.o CarlaBridgeToolkitQt__lv2-qt5.o

carla-bridge-lv2-qt5: $(OBJS_UI_LV2_QT5) $(OBJS_UI_LV2_LIBS)
	$(CXX) $^ $(LINK_UI_LV2_QT5_FLAGS) -o $@ && $(STRIP) $@

%__lv2-qt5.o: %.cpp
	$(CXX) $< $(BUILD_UI_LV2_QT5_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-x11

OBJS_UI_LV2_X11 = CarlaBridgeUI-LV2__lv2-x11.o \
	CarlaBridgeClient__lv2.o CarlaBridgeOsc__lv2.o \
	CarlaBridgeToolkit__lv2.o CarlaBridgeToolkitQt__lv2-x11.o

carla-bridge-lv2-x11: $(OBJS_UI_LV2_X11) $(OBJS_UI_LV2_LIBS)
	$(CXX) $^ $(LINK_UI_LV2_X11_FLAGS) -o $@ && $(STRIP) $@

%__lv2-x11.o: %.cpp
	$(CXX) $< $(BUILD_UI_LV2_X11_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_vst-hwnd

OBJS_UI_VST_HWND = CarlaBridgeUI-VST__vst-hwnd.o \
	CarlaBridgeClient__vst.o CarlaBridgeOsc__vst.o \
	CarlaBridgeToolkit__vst.o CarlaBridgeToolkitQt__vst-hwnd.o

carla-bridge-vst-hwnd.exe: $(OBJS_UI_VST_HWND)
	$(CXX) $^ $(LINK_UI_VST_HWND_FLAGS) -o $@ && $(STRIP) $@

%__vst-hwnd.o: %.cpp
	$(CXX) $< $(BUILD_UI_VST_HWND_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_vst-x11

OBJS_UI_VST_X11 = CarlaBridgeUI-VST__vst-x11.o \
	CarlaBridgeClient__vst.o CarlaBridgeOsc__vst.o \
	CarlaBridgeToolkit__vst.o CarlaBridgeToolkitQt__vst-x11.o

carla-bridge-vst-x11: $(OBJS_UI_VST_X11)
	$(CXX) $^ $(LINK_UI_VST_X11_FLAGS) -o $@ && $(STRIP) $@

%__vst-x11.o: %.cpp
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
	../backend/engine/CarlaEngineJack__native.o

# carla-plugin
OBJS_NATIVE += \
	../backend/plugin/CarlaPlugin__native.o \
	../backend/plugin/CarlaPluginThread__native.o \
	../backend/plugin/NativePlugin__native.o \
	../backend/plugin/LadspaPlugin__native.o \
	../backend/plugin/DssiPlugin__native.o \
	../backend/plugin/Lv2Plugin__native.o \
	../backend/plugin/VstPlugin__native.o \
	../backend/plugin/Vst3Plugin__native.o \
	../backend/plugin/FluidSynthPlugin__native.o \
	../backend/plugin/LinuxSamplerPlugin__native.o

# carla-standalone
OBJS_NATIVE += \
	../backend/standalone/CarlaStandalone__native.o

# libs
OBJS_NATIVE += \
	../backend/libcarla_native.a \
	../libs/dgl.a \
	../libs/lilv.a \
	../libs/rtmempool.a

carla-bridge-native: $(OBJS_NATIVE)
	$(CXX) $^ $(NATIVE_LINK_FLAGS) -o $@ && $(STRIP) $@

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
	../backend/engine/CarlaEngineJack__posix32.o

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
	../libs/dgl.posix32.a \
	../libs/lilv.posix32.a \
	../libs/rtmempool.posix32.a

carla-bridge-posix32: $(OBJS_POSIX32)
	$(CXX) $^ $(POSIX_LINK_FLAGS) $(POSIX_32BIT_FLAGS) -o $@ && $(STRIP) $@

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
	../backend/engine/CarlaEngineJack__posix64.o

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
	../libs/dgl.posix64.a \
	../libs/lilv.posix64.a \
	../libs/rtmempool.posix64.a

carla-bridge-posix64: $(OBJS_POSIX64)
	$(CXX) $^ $(POSIX_LINK_FLAGS) $(POSIX_64BIT_FLAGS) -o $@ && $(STRIP) $@

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
	../libs/dgl.win32.a \
	../libs/lilv.win32.a \
	../libs/rtmempool.win32.a

carla-bridge-win32.exe: $(OBJS_WIN32)  ../libs/jackbridge-win32.dll
	$(CXX) $(OBJS_WIN32) $(WIN_32BIT_FLAGS) $(WIN_LINK_FLAGS) -ljackbridge-win32 -o $@ && $(STRIP) $@

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
	../libs/dgl.win64.a \
	../libs/lilv.win64.a \
	../libs/rtmempool.win64.a

carla-bridge-win64.exe: $(OBJS_WIN64) ../libs/jackbridge-win64.dll
	$(CXX) $(OBJS_WIN64) $(WIN_64BIT_FLAGS) $(WIN_LINK_FLAGS) -ljackbridge-win64 -o $@ && $(STRIP) $@

%__win64.o: %.cpp
	$(CXX) $< $(WIN_BUILD_FLAGS) $(WIN_64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

../backend/libcarla_native.a:
	$(MAKE) -C ../backend/native

../libs/%:
	$(MAKE) -C ../libs $*

# --------------------------------------------------------------

doxygen: carla_bridge.doxygen
	doxygen $<

clean:
	rm -f *.o *.dll *.so *.exe
	rm -f $(OBJS_NATIVE)
	rm -f $(OBJS_POSIX32)
	rm -f $(OBJS_POSIX64)
	rm -f $(OBJS_WIN32)
	rm -f $(OBJS_WIN64)
	rm -f carla-bridge-lv2-gtk2 carla-bridge-lv2-gtk3 carla-bridge-lv2-qt4 carla-bridge-lv2-qt5 carla-bridge-lv2-x11 carla-bridge-vst-x11
	rm -f carla-bridge-native carla-bridge-posix32 carla-bridge-posix64
