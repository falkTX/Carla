# QtCreator project file

QT = core gui

CONFIG    = debug
CONFIG   += link_pkgconfig qt shared warn_on

DEFINES   = DEBUG
DEFINES  += QTCREATOR_TEST

# Plugins
DEFINES  += WANT_LADSPA
# WANT_DSSI WANT_LV2 WANT_VST

# Samplers
#DEFINES  += WANT_FLUIDSYNTH WANT_LINUXSAMPLER

# ZynAddSubFX
DEFINES  += WANT_ZYNADDSUBFX

# Misc
DEFINES  += WANT_SUIL

PKGCONFIG = liblo suil-0 fluidsynth linuxsampler

TARGET   = carla_plugin
TEMPLATE = lib
VERSION  = 0.5.0

SOURCES = \
    carla_plugin.cpp \
    carla_plugin_thread.cpp \
    carla_bridge.cpp \
    native.cpp \
    ladspa.cpp \
    dssi.cpp \
    lv2.cpp \
    vst.cpp \
    fluidsynth.cpp \
    linuxsampler.cpp

HEADERS = \
    carla_plugin_internal.hpp \
    carla_plugin_thread.hpp

HEADERS += \
    ../carla_backend.hpp \
    ../carla_engine.hpp \
    ../carla_native.h \
    ../carla_plugin.hpp

INCLUDEPATH = . .. \
    ../../includes \
    ../../libs \
    ../../utils

# FIXME
INCLUDEPATH += \
    /opt/kxstudio/include

QMAKE_CXXFLAGS *= -std=c++0x
