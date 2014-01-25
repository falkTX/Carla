# QtCreator project file

TARGET   = carla-bridge-qtcreator
TEMPLATE = app

# -------------------------------------------------------

CONFIG     = debug
CONFIG    += link_pkgconfig warn_on

DEFINES    = DEBUG
DEFINES   += HAVE_CPP11_SUPPORT
DEFINES   += QTCREATOR_TEST

DEFINES   += BUILD_BRIDGE
DEFINES   += BUILD_BRIDGE_PLUGIN

# Shared
DEFINES   += WANT_NATIVE
DEFINES   += WANT_LADSPA
DEFINES   += WANT_DSSI
DEFINES   += WANT_LV2
DEFINES   += WANT_VST
DEFINES   += WANT_AU
DEFINES   += WANT_CSOUND
DEFINES   += WANT_FLUIDSYNTH
DEFINES   += WANT_LINUXSAMPLER
DEFINES   += WANT_OPENGL
DEFINES   += WANT_AUDIOFILE
DEFINES   += WANT_MIDIFILE
DEFINES   += WANT_ZYNADDSUBFX
DEFINES   += WANT_ZYNADDSUBFX_UI

# Engine
PKGCONFIG  = liblo

# FluidSynth
PKGCONFIG += fluidsynth

# LinuxSampler
PKGCONFIG += linuxsampler

# AudioFile
# DEFINES   += HAVE_FFMPEG
# PKGCONFIG += libavcodec libavformat libavutil sndfile

# MidiFile
PKGCONFIG += smf

# OpenGL
PKGCONFIG += gl

# ZynAddSubFX
PKGCONFIG += fftw3 mxml zlib ntk ntk_images

# -----------------------------------------------------------

SOURCES = \
    ../CarlaBridgeClient.cpp \
    ../CarlaBridgeOsc.cpp \
    ../CarlaBridgePlugin.cpp

HEADERS = \
    ../CarlaBridge.hpp \
    ../CarlaBridgeClient.hpp \
    ../CarlaBridgeOsc.hpp

# -----------------------------------------------------------

# Engine
SOURCES += \
    ../../backend/engine/CarlaEngine.cpp \
    ../../backend/engine/CarlaEngineOsc.cpp \
    ../../backend/engine/CarlaEngineThread.cpp \
    ../../backend/engine/CarlaEngineBridge.cpp \
    ../../backend/engine/CarlaEngineJack.cpp \
    ../../backend/engine/CarlaEngineNative.cpp
#    ../../backend/engine/CarlaEngineJuce.cpp \
#    ../../backend/engine/CarlaEngineRtAudio.cpp

# Plugin
SOURCES += \
    ../../backend/plugin/CarlaPlugin.cpp \
    ../../backend/plugin/CarlaPluginThread.cpp \
    ../../backend/plugin/BridgePlugin.cpp \
    ../../backend/plugin/NativePlugin.cpp \
    ../../backend/plugin/LadspaPlugin.cpp \
    ../../backend/plugin/DssiPlugin.cpp \
    ../../backend/plugin/Lv2Plugin.cpp \
    ../../backend/plugin/VstPlugin.cpp \
    ../../backend/plugin/CsoundPlugin.cpp \
    ../../backend/plugin/FluidSynthPlugin.cpp \
    ../../backend/plugin/LinuxSamplerPlugin.cpp

# Standalone
SOURCES += \
    ../../backend/standalone/CarlaStandalone.cpp

# -----------------------------------------------------------

# common
HEADERS += \
    ../../backend/*.hpp

# engine
HEADERS += \
    ../../backend/engine/*.hpp

# plugin
HEADERS += \
    ../../backend/plugin/*.hpp

# includes
HEADERS += \
    ../../includes/*.h \
    ../../includes/*.hpp

# modules
HEADERS += \
    ../../modules/*.h \
    ../../modules/*.hpp

# utils
HEADERS += \
    ../../utils/*.hpp

INCLUDEPATH = .. \
    ../../backend \
    ../../backend/engine \
    ../../backend/plugin \
    ../../includes \
    ../../modules \
    ../../utils

# -----------------------------------------------------------

LIBS  = -ldl -lpthread -lrt
LIBS += ../../modules/daz.a
LIBS += ../../modules/juce_audio_basics.a
LIBS += ../../modules/juce_core.a
LIBS += ../../modules/juce_data_structures.a
LIBS += ../../modules/juce_events.a
LIBS += ../../modules/rtmempool.a

LIBS += ../../modules/dgl.a
LIBS += ../../modules/lilv.a

QMAKE_CXXFLAGS *= -std=gnu++0x
