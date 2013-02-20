# QtCreator project file

QT = core gui

CONFIG    = debug
CONFIG   += link_pkgconfig shared qt warn_on

DEFINES   = DEBUG
DEFINES  += QTCREATOR_TEST

# Plugins
DEFINES  += WANT_LADSPA WANT_DSSI
# WANT_LV2 WANT_VST

# Samplers
DEFINES  += WANT_FLUIDSYNTH
# WANT_LINUXSAMPLER

# ZynAddSubFX
DEFINES  += WANT_ZYNADDSUBFX

# Misc
DEFINES  += WANT_SUIL

PKGCONFIG = liblo suil-0 fluidsynth linuxsampler

TARGET   = CarlaPlugin
TEMPLATE = lib
VERSION  = 0.5.0

SOURCES  = \
    CarlaPlugin.cpp \
    CarlaPluginThread.cpp \
    CarlaBridge.cpp \
    NativePlugin.cpp \
    LadspaPlugin.cpp \
    DssiPlugin.cpp \
    Lv2Plugin.cpp \
    VstPlugin.cpp \
    FluidSynthPlugin.cpp \
    LinuxSamplerPlugin.cpp

HEADERS  = \
    CarlaPluginInternal.hpp \
    CarlaPluginThread.hpp

HEADERS += \
    ../CarlaBackend.hpp \
    ../CarlaEngine.hpp \
    ../CarlaNative.h \
    ../CarlaPlugin.hpp

HEADERS += \
    ../../utils/CarlaUtils.hpp \
    ../../utils/CarlaJuceUtils.hpp \
    ../../utils/CarlaLibUtils.hpp \
    ../../utils/CarlaOscUtils.hpp \
    ../../utils/CarlaStateUtils.hpp \
    ../../utils/CarlaMutex.hpp \
    ../../utils/CarlaString.hpp \
    ../../utils/CarlaThread.hpp

INCLUDEPATH = . .. \
    ../../includes \
    ../../libs \
    ../../utils

QMAKE_CXXFLAGS += -std=c++0x
