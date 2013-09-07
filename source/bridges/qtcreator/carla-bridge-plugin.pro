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
# DEFINES   += WANT_ZYNADDSUBFX_UI

# Engine
PKGCONFIG  = liblo

# FluidSynth
PKGCONFIG += fluidsynth

# LinuxSampler
PKGCONFIG += linuxsampler

# AudioFile
DEFINES   += HAVE_FFMPEG
PKGCONFIG += libavcodec libavformat libavutil sndfile

# MidiFile
PKGCONFIG += smf

# OpenGL
PKGCONFIG += gl

# ZynAddSubFX
PKGCONFIG += fftw3 mxml zlib
# ntk ntk_images

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
    ../../backend/plugin/CarlaPluginGui.cpp \
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
    ../../backend/CarlaBackend.hpp \
    ../../backend/CarlaEngine.hpp \
    ../../backend/CarlaPlugin.hpp \
    ../../backend/CarlaHost.hpp

# engine
HEADERS += \
    ../../backend/engine/CarlaEngineInternal.hpp \
    ../../backend/engine/CarlaEngineOsc.hpp \
    ../../backend/engine/CarlaEngineThread.hpp

# plugin
HEADERS += \
    ../../backend/plugin/CarlaPluginInternal.hpp \
    ../../backend/plugin/CarlaPluginGui.hpp \
    ../../backend/plugin/CarlaPluginThread.hpp

# includes
HEADERS += \
    ../../includes/CarlaDefines.hpp \
    ../../includes/CarlaMIDI.h

# modules
HEADERS += \
    ../../modules/CarlaNative.h \
    ../../modules/CarlaNative.hpp \
    ../../modules/juce_audio_basics.h \
    ../../modules/juce_core.h \
    ../../modules/ladspa_rdf.hpp \
    ../../modules/lv2_rdf.hpp

# utils
HEADERS += \
    ../../utils/CarlaUtils.hpp \
    ../../utils/CarlaBackendUtils.hpp \
    ../../utils/CarlaBridgeUtils.hpp \
    ../../utils/CarlaDssiUtils.hpp \
    ../../utils/CarlaJuceUtils.hpp \
    ../../utils/CarlaLadspaUtils.hpp \
    ../../utils/CarlaLibUtils.hpp \
    ../../utils/CarlaLogThread.hpp \
    ../../utils/CarlaLv2Utils.hpp \
    ../../utils/CarlaOscUtils.hpp \
    ../../utils/CarlaPipeUtils.hpp \
    ../../utils/CarlaShmUtils.hpp \
    ../../utils/CarlaStateUtils.hpp \
    ../../utils/CarlaVstUtils.hpp \
    ../../utils/CarlaMutex.hpp \
    ../../utils/CarlaRingBuffer.hpp \
    ../../utils/CarlaString.hpp \
    ../../utils/Lv2AtomQueue.hpp \
    ../../utils/RtList.hpp

INCLUDEPATH = .. \
    ../../backend \
    ../../backend/engine \
    ../../backend/plugin \
    ../../includes \
    ../../modules \
    ../../utils

# -----------------------------------------------------------

LIBS  = -ldl -lpthread -lrt
LIBS += ../../modules/carla_native.a
LIBS += ../../modules/juce_audio_basics.a
LIBS += ../../modules/juce_core.a
LIBS += ../../modules/rtmempool.a

LIBS += ../../modules/dgl.a
LIBS += ../../modules/lilv.a

QMAKE_CXXFLAGS *= -std=gnu++0x
