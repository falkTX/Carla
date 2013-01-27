#!/usr/bin/make -f
# Makefile for carla-bridges #
# ---------------------------------------- #
# Created by falkTX
#

include ../Makefile.mk

HAVE_GTK2        = $(shell pkg-config --exists gtk+-2.0 && echo true)
HAVE_GTK3        = $(shell pkg-config --exists gtk+-3.0 && echo true)

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -DBUILD_BRIDGE
BUILD_CXX_FLAGS += -I. -I../carla-includes -I../carla-utils
BUILD_CXX_FLAGS += $(shell pkg-config --cflags liblo QtCore QtXml)
LINK_FLAGS      += $(shell pkg-config --libs liblo QtCore QtXml)

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

BUILD_PLUGIN_FLAGS  = $(BUILD_CXX_FLAGS) -DBUILD_BRIDGE_PLUGIN -DBRIDGE_PLUGIN
BUILD_PLUGIN_FLAGS += -DCARLA_ENGINE_JACK -I../carla-backend -I../carla-engine -I../carla-jackbridge -I../carla-plugin
BUILD_PLUGIN_FLAGS += $(shell pkg-config --cflags jack) $(QT_UI_FLAGS)

LINK_PLUGIN_FLAGS   = $(LINK_FLAGS)
LINK_PLUGIN_FLAGS  += $(QT_UI_LIBS)

POSIX_BUILD_FLAGS   = $(BUILD_PLUGIN_FLAGS)
POSIX_32BIT_FLAGS   = $(32BIT_FLAGS) -L/usr/lib32 -L/usr/lib/i386-linux-gnu
POSIX_64BIT_FLAGS   = $(64BIT_FLAGS) -L/usr/lib64 -L/usr/lib/x86_64-linux-gnu
POSIX_LINK_FLAGS    = $(LINK_PLUGIN_FLAGS) $(shell pkg-config --libs jack) -ldl

WIN_BUILD_FLAGS     = $(BUILD_PLUGIN_FLAGS) -DJACKBRIDGE_EXPORT -DPTW32_STATIC_LIB
WIN_32BIT_FLAGS     = $(32BIT_FLAGS)
WIN_64BIT_FLAGS     = $(64BIT_FLAGS)
WIN_LINK_FLAGS      = $(LINK_PLUGIN_FLAGS) -mwindows -L../carla-jackbridge -lpthread -lwinspool -lole32 -luuid -limm32 -lshell32 -lws2_32

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
	../carla-lilv/carla_lilv.a \
	../carla-rtmempool/carla_rtmempool.a

# --------------------------------------------------------------
# ui_lv2-gtk2

OBJS_UI_LV2_GTK2 = carla_bridge_ui-lv2__lv2-gtk2.o \
	carla_bridge_client__lv2.o carla_bridge_osc__lv2.o \
	carla_bridge_toolkit__lv2.o carla_bridge_toolkit-gtk__lv2-gtk2.o

carla-bridge-lv2-gtk2: $(OBJS_UI_LV2_GTK2) $(OBJS_UI_LV2_LIBS)
	$(CXX) $^ $(LINK_UI_LV2_GTK2_FLAGS) -o $@ && $(STRIP) $@

%__lv2-gtk2.o: %.cpp
	$(CXX) $< $(BUILD_UI_LV2_GTK2_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-gtk3

OBJS_UI_LV2_GTK3 = carla_bridge_ui-lv2__lv2-gtk3.o \
	carla_bridge_client__lv2.o carla_bridge_osc__lv2.o \
	carla_bridge_toolkit__lv2.o carla_bridge_toolkit-gtk__lv2-gtk3.o

carla-bridge-lv2-gtk3: $(OBJS_UI_LV2_GTK3) $(OBJS_UI_LV2_LIBS)
	$(CXX) $^ $(LINK_UI_LV2_GTK3_FLAGS) -o $@ && $(STRIP) $@

%__lv2-gtk3.o: %.cpp
	$(CXX) $< $(BUILD_UI_LV2_GTK3_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-qt4

OBJS_UI_LV2_QT4 = carla_bridge_ui-lv2__lv2-qt4.o \
	carla_bridge_client__lv2.o carla_bridge_osc__lv2.o \
	carla_bridge_toolkit__lv2.o carla_bridge_toolkit-qt__lv2-qt4.o

carla-bridge-lv2-qt4: $(OBJS_UI_LV2_QT4) $(OBJS_UI_LV2_LIBS)
	$(CXX) $^ $(LINK_UI_LV2_QT4_FLAGS) -o $@ && $(STRIP) $@

%__lv2-qt4.o: %.cpp
	$(CXX) $< $(BUILD_UI_LV2_QT4_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-qt5

OBJS_UI_LV2_QT5 = carla_bridge_ui-lv2__lv2-qt5.o \
	carla_bridge_client__lv2.o carla_bridge_osc__lv2.o \
	carla_bridge_toolkit__lv2.o carla_bridge_toolkit-qt__lv2-qt5.o

carla-bridge-lv2-qt5: $(OBJS_UI_LV2_QT5) $(OBJS_UI_LV2_LIBS)
	$(CXX) $^ $(LINK_UI_LV2_QT5_FLAGS) -o $@ && $(STRIP) $@

%__lv2-qt5.o: %.cpp
	$(CXX) $< $(BUILD_UI_LV2_QT5_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-x11

OBJS_UI_LV2_X11 = carla_bridge_ui-lv2__lv2-x11.o \
	carla_bridge_client__lv2.o carla_bridge_osc__lv2.o \
	carla_bridge_toolkit__lv2.o carla_bridge_toolkit-qt__lv2-x11.o

carla-bridge-lv2-x11: $(OBJS_UI_LV2_X11) $(OBJS_UI_LV2_LIBS)
	$(CXX) $^ $(LINK_UI_LV2_X11_FLAGS) -o $@ && $(STRIP) $@

%__lv2-x11.o: %.cpp
	$(CXX) $< $(BUILD_UI_LV2_X11_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_vst-hwnd

OBJS_UI_VST_HWND = carla_bridge_ui-vst__vst-hwnd.o \
	carla_bridge_client__vst.o carla_bridge_osc__vst.o \
	carla_bridge_toolkit__vst.o carla_bridge_toolkit-qt__vst-hwnd.o

carla-bridge-vst-hwnd.exe: $(OBJS_UI_VST_HWND)
	$(CXX) $^ $(LINK_UI_VST_HWND_FLAGS) -o $@ && $(STRIP) $@

%__vst-hwnd.o: %.cpp
	$(CXX) $< $(BUILD_UI_VST_HWND_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_vst-x11

OBJS_UI_VST_X11 = carla_bridge_ui-vst__vst-x11.o \
	carla_bridge_client__vst.o carla_bridge_osc__vst.o \
	carla_bridge_toolkit__vst.o carla_bridge_toolkit-qt__vst-x11.o

carla-bridge-vst-x11: $(OBJS_UI_VST_X11)
	$(CXX) $^ $(LINK_UI_VST_X11_FLAGS) -o $@ && $(STRIP) $@

%__vst-x11.o: %.cpp
	$(CXX) $< $(BUILD_UI_VST_X11_FLAGS) -c -o $@

# --------------------------------------------------------------
# native

NATIVE_BUILD_FLAGS  = $(BUILD_PLUGIN_FLAGS)
NATIVE_LINK_FLAGS   = $(LINK_PLUGIN_FLAGS) $(shell pkg-config --libs jack) -ldl

ifeq ($(HAVE_SUIL),true)
NATIVE_BUILD_FLAGS += $(shell pkg-config --cflags suil-0) -DWANT_SUIL
NATIVE_LINK_FLAGS  += $(shell pkg-config --libs suil-0)
endif

OBJS_NATIVE  = carla_bridge_plugin__native.o \
	carla_bridge_client__native.o carla_bridge_osc__native.o \
	carla_bridge_toolkit__native.o

# carla
OBJS_NATIVE += \
	../carla/Shared__native.o

# carla-engine
OBJS_NATIVE += \
	../carla-engine/carla_engine__native.o \
	../carla-engine/carla_engine_osc__native.o \
	../carla-engine/carla_engine_thread__native.o \
	../carla-engine/jack__native.o

# carla-plugin
OBJS_NATIVE += \
	../carla-plugin/carla_plugin__native.o \
	../carla-plugin/carla_plugin_thread__native.o \
	../carla-plugin/ladspa__native.o \
	../carla-plugin/dssi__native.o \
	../carla-plugin/lv2__native.o \
	../carla-plugin/vst__native.o

# libs
OBJS_NATIVE += \
	../carla-lilv/carla_lilv.a \
	../carla-rtmempool/carla_rtmempool.a

carla-bridge-native: $(OBJS_NATIVE)
	$(CXX) $^ $(NATIVE_LINK_FLAGS) -o $@ && $(STRIP) $@

%__native.o: %.cpp
	$(CXX) $< $(NATIVE_BUILD_FLAGS) -c -o $@

# --------------------------------------------------------------
# posix32

OBJS_POSIX32  = carla_bridge_plugin__posix32.o \
	carla_bridge_client__posix32.o carla_bridge_osc__posix32.o \
	carla_bridge_toolkit__posix32.o

# carla
OBJS_POSIX32 += \
	../carla/Shared__posix32.o

# carla-engine
OBJS_POSIX32 += \
	../carla-engine/carla_engine__posix32.o \
	../carla-engine/carla_engine_osc__posix32.o \
	../carla-engine/carla_engine_thread__posix32.o \
	../carla-engine/jack__posix32.o

# carla-plugin
OBJS_POSIX32 += \
	../carla-plugin/carla_plugin__posix32.o \
	../carla-plugin/carla_plugin_thread__posix32.o \
	../carla-plugin/ladspa__posix32.o \
	../carla-plugin/dssi__posix32.o \
	../carla-plugin/lv2__posix32.o \
	../carla-plugin/vst__posix32.o

# libs
OBJS_POSIX32 += \
	../carla-lilv/carla_lilv_posix32.a \
	../carla-rtmempool/carla_rtmempool_posix32.a

carla-bridge-posix32: $(OBJS_POSIX32)
	$(CXX) $^ $(POSIX_LINK_FLAGS) $(POSIX_32BIT_FLAGS) -o $@ && $(STRIP) $@

%__posix32.o: %.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_32BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
# posix64

OBJS_POSIX64  = carla_bridge_plugin__posix64.o \
	carla_bridge_client__posix64.o carla_bridge_osc__posix64.o \
	carla_bridge_toolkit__posix64.o

# carla
OBJS_POSIX64 += \
	../carla/Shared__posix64.o

# carla-engine
OBJS_POSIX64 += \
	../carla-engine/carla_engine__posix64.o \
	../carla-engine/carla_engine_osc__posix64.o \
	../carla-engine/carla_engine_thread__posix64.o \
	../carla-engine/jack__posix64.o

# carla-plugin
OBJS_POSIX64 += \
	../carla-plugin/carla_plugin__posix64.o \
	../carla-plugin/carla_plugin_thread__posix64.o \
	../carla-plugin/ladspa__posix64.o \
	../carla-plugin/dssi__posix64.o \
	../carla-plugin/lv2__posix64.o \
	../carla-plugin/vst__posix64.o

# libs
OBJS_POSIX64 += \
	../carla-lilv/carla_lilv_posix64.a \
	../carla-rtmempool/carla_rtmempool_posix64.a

carla-bridge-posix64: $(OBJS_POSIX64)
	$(CXX) $^ $(POSIX_LINK_FLAGS) $(POSIX_64BIT_FLAGS) -o $@ && $(STRIP) $@

%__posix64.o: %.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
# win32

OBJS_WIN32 = carla_bridge_plugin__win32.o \
	carla_bridge_client__win32.o carla_bridge_osc__win32.o \
	carla_bridge_toolkit__win32.o

# carla
OBJS_WIN32 += \
	../carla/Shared__win32.o

# carla-engine
OBJS_WIN32 += \
	../carla-engine/carla_engine__win32.o \
	../carla-engine/carla_engine_osc__win32.o \
	../carla-engine/carla_engine_thread__win32.o \
	../carla-engine/jack__win32.o

# carla-plugin
OBJS_WIN32 += \
	../carla-plugin/carla_plugin__win32.o \
	../carla-plugin/carla_plugin_thread__win32.o \
	../carla-plugin/ladspa__win32.o \
	../carla-plugin/dssi__win32.o \
	../carla-plugin/lv2__win32.o \
	../carla-plugin/vst__win32.o

# libs
OBJS_WIN32 += \
	../carla-lilv/carla_lilv_win32.a \
	../carla-rtmempool/carla_rtmempool_win32.a

carla-bridge-win32.exe: $(OBJS_WIN32) ../carla-jackbridge/libcarla-jackbridge-win32.dll
	$(CXX) $(OBJS_WIN32) $(WIN_32BIT_FLAGS) $(WIN_LINK_FLAGS) -lcarla-jackbridge-win32 -o $@ && $(STRIP) $@

%__win32.o: %.cpp
	$(CXX) $< $(WIN_BUILD_FLAGS) $(WIN_32BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
# win64

OBJS_WIN64 = carla_bridge_plugin__win64.o \
	carla_bridge_client__win64.o carla_bridge_osc__win64.o \
	carla_bridge_toolkit__win64.o

# carla
OBJS_WIN64 += \
	../carla/Shared__win64.o

# carla-engine
OBJS_WIN64 += \
	../carla-engine/carla_engine__win64.o \
	../carla-engine/carla_engine_osc__win64.o \
	../carla-engine/carla_engine_thread__win64.o \
	../carla-engine/jack__win64.o

# carla-plugin
OBJS_WIN64 += \
	../carla-plugin/carla_plugin__win64.o \
	../carla-plugin/carla_plugin_thread__win64.o \
	../carla-plugin/ladspa__win64.o \
	../carla-plugin/dssi__win64.o \
	../carla-plugin/lv2__win64.o \
	../carla-plugin/vst__win64.o

# libs
OBJS_WIN64 += \
	../carla-lilv/carla_lilv_win64.a \
	../carla-rtmempool/carla_rtmempool_win64.a

carla-bridge-win64.exe: $(OBJS_WIN64) ../carla-jackbridge/libcarla-jackbridge-win64.dll
	$(CXX) $(OBJS_WIN64) $(WIN_64BIT_FLAGS) $(WIN_LINK_FLAGS) -lcarla-jackbridge-win64 -o $@ && $(STRIP) $@

%__win64.o: %.cpp
	$(CXX) $< $(WIN_BUILD_FLAGS) $(WIN_64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

../carla-lilv/carla_lilv.a:
	$(MAKE) -C ../carla-lilv

../carla-lilv/carla_lilv_posix32.a:
	$(MAKE) -C ../carla-lilv posix32

../carla-lilv/carla_lilv_posix64.a:
	$(MAKE) -C ../carla-lilv posix64

../carla-lilv/carla_lilv_win32.a:
	$(MAKE) -C ../carla-lilv win32

../carla-lilv/carla_lilv_win64.a:
	$(MAKE) -C ../carla-lilv win64

../carla-rtmempool/carla_rtmempool.a:
	$(MAKE) -C ../carla-rtmempool

../carla-rtmempool/carla_rtmempool_posix32.a:
	$(MAKE) -C ../carla-rtmempool posix32

../carla-rtmempool/carla_rtmempool_posix64.a:
	$(MAKE) -C ../carla-rtmempool posix64

../carla-rtmempool/carla_rtmempool_win32.a:
	$(MAKE) -C ../carla-rtmempool win32

../carla-rtmempool/carla_rtmempool_win64.a:
	$(MAKE) -C ../carla-rtmempool win64

../carla-jackbridge/libcarla-jackbridge-win32.dll:
	$(MAKE) -C ../carla-jackbridge win32

../carla-jackbridge/libcarla-jackbridge-win64.dll:
	$(MAKE) -C ../carla-jackbridge win64

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
