TARGET   = carla_backend
TEMPLATE = lib
VERSION  = 1.9.5

mac {
QT_CONFIG -= no-pkg-config
PKG_CONFIG = PKG_CONFIG_PATH=/Users/falktx/builds/carla/lib/pkgconfig:/Users/falktx/builds/carla64/lib/pkgconfig /Users/falktx/builds/carla64/bin/pkg-config
}

CONFIG  = debug
CONFIG += link_pkgconfig shared warn_on

DEFINES  = DEBUG
DEFINES += BUILDING_CARLA
DEFINES += REAL_BUILD
# DEFINES += HAVE_DGL
DEFINES += HAVE_LIBLO
DEFINES += HAVE_LIBMAGIC
DEFINES += HAVE_FLUIDSYNTH
DEFINES += HAVE_LINUXSAMPLER
DEFINES += HAVE_ZYN_DEPS
DEFINES += HAVE_ZYN_UI_DEPS

PKGCONFIG += liblo
PKGCONFIG += fftw3
PKGCONFIG += fluidsynth
PKGCONFIG += mxml
PKGCONFIG += zlib

SOURCES = \
# Backend (main)
    backend/CarlaStandalone.cpp \
# Backend (engine)
    backend/engine/CarlaEngine.cpp \
    backend/engine/CarlaEngineClient.cpp \
    backend/engine/CarlaEngineData.cpp \
    backend/engine/CarlaEngineInternal.cpp \
    backend/engine/CarlaEngineGraph.cpp \
    backend/engine/CarlaEngineJack.cpp \
    backend/engine/CarlaEngineJuce.cpp \
    backend/engine/CarlaEngineNative.cpp \
    backend/engine/CarlaEngineOsc.cpp \
    backend/engine/CarlaEngineOscSend.cpp \
    backend/engine/CarlaEnginePorts.cpp \
    backend/engine/CarlaEngineThread.cpp \
# Backend (plugins)
    backend/plugin/CarlaPlugin.cpp \
    backend/plugin/CarlaPluginAU.cpp \
    backend/plugin/CarlaPluginBridge.cpp \
    backend/plugin/CarlaPluginDSSI.cpp \
    backend/plugin/CarlaPluginFluidSynth.cpp \
    backend/plugin/CarlaPluginInternal.cpp \
    backend/plugin/CarlaPluginJuce.cpp \
    backend/plugin/CarlaPluginLADSPA.cpp \
    backend/plugin/CarlaPluginLV2.cpp \
    backend/plugin/CarlaPluginNative.cpp \
    backend/plugin/CarlaPluginVST2.cpp

HEADERS = \
# C API
    backend/CarlaBackend.h \
    backend/CarlaHost.h \
    backend/CarlaUtils.h \
# C++ API
    backend/CarlaEngine.hpp \
    backend/CarlaPlugin.hpp

INCLUDEPATH = \
    backend \
    includes \
    modules \
    utils

LIBS = \
# Pre-Compiled modules
    ../build/modules/Debug/jackbridge.a \
    ../build/modules/Debug/juce_core.a \
    ../build/modules/Debug/lilv.a \
    ../build/modules/Debug/native-plugins.a \
    ../build/modules/Debug/rtmempool.a

#mac {
LIBS += \
# Pre-Compiled modules (OSX only)
    ../build/modules/Debug/juce_gui_extra.a \
# OSX frameworks
    -framework Accelerate \
    -framework AppKit \
    -framework AudioToolbox \
    -framework AudioUnit \
    -framework Cocoa \
    -framework CoreAudio \
    -framework CoreAudioKit \
    -framework CoreFoundation \
    -framework CoreMIDI \
    -framework IOKit \
    -framework OpenGL \
    -framework QuartzCore
#}

#unix {
LIBS += \
    -lmagic
#}

QMAKE_CFLAGS *= -std=gnu99
QMAKE_CXXFLAGS *= -std=gnu++0x
