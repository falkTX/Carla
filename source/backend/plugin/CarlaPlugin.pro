# QtCreator project file

TARGET   = CarlaPlugin
TEMPLATE = lib
VERSION  = 1.0

# -------------------------------------------------------

QT = core gui xml

CONFIG     = debug
CONFIG    += link_pkgconfig qt shared warn_on

DEFINES    = DEBUG
DEFINES   += QTCREATOR_TEST

# Shared
DEFINES   += WANT_NATIVE
DEFINES   += WANT_LADSPA
DEFINES   += WANT_DSSI
DEFINES   += WANT_LV2
DEFINES   += WANT_VST
DEFINES   += WANT_PLUGIN
DEFINES   += WANT_RTAUDIO
DEFINES   += WANT_JACK
DEFINES   += WANT_JACK_LATENCY
DEFINES   += WANT_JACK_PORT_RENAME
DEFINES   += WANT_FLUIDSYNTH
DEFINES   += WANT_LINUXSAMPLER
DEFINES   += WANT_OPENGL
DEFINES   += WANT_AUDIOFILE
DEFINES   += WANT_MIDIFILE
DEFINES   += WANT_ZYNADDSUBFX
DEFINES   += WANT_ZYNADDSUBFX_UI

# Plugin
PKGCONFIG += liblo

# FluidSynth
PKGCONFIG += fluidsynth linuxsampler

# LinuxSampler
PKGCONFIG += linuxsampler

# -------------------------------------------------------

SOURCES  = \
    CarlaPlugin.cpp \
    CarlaPluginGui.cpp \
    CarlaPluginThread.cpp \
    BridgePlugin.cpp \
    NativePlugin.cpp \
    LadspaPlugin.cpp \
    DssiPlugin.cpp \
    Lv2Plugin.cpp \
    VstPlugin.cpp \
    FluidSynthPlugin.cpp \
    LinuxSamplerPlugin.cpp

HEADERS  = \
    CarlaPluginInternal.hpp \
    CarlaPluginGui.hpp \
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
    ../../utils/CarlaString.hpp

INCLUDEPATH = . .. \
    ../../includes \
    ../../libs \
    ../../utils

QMAKE_CXXFLAGS += -std=c++0x
