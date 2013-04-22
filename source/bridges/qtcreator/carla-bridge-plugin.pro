# QtCreator project file

TARGET   = carla-bridge-qtcreator
TEMPLATE = app
VERSION  = 1.0

# -------------------------------------------------------

QT = core gui xml

CONFIG     = debug
CONFIG    += link_pkgconfig qt shared warn_on

DEFINES    = DEBUG
DEFINES   += HAVE_CPP11_SUPPORT
DEFINES   += QTCREATOR_TEST

DEFINES   += BUILD_BRIDGE BUILD_BRIDGE_PLUGIN BRIDGE_PLUGIN

# Shared
DEFINES   += WANT_NATIVE
DEFINES   += WANT_LADSPA
DEFINES   += WANT_DSSI
DEFINES   += WANT_LV2
DEFINES   += WANT_VST
#DEFINES   += WANT_PLUGIN
#DEFINES   += WANT_RTAUDIO
DEFINES   += WANT_FLUIDSYNTH
DEFINES   += WANT_LINUXSAMPLER
DEFINES   += WANT_OPENGL
DEFINES   += WANT_AUDIOFILE
DEFINES   += WANT_MIDIFILE
DEFINES   += WANT_ZYNADDSUBFX
DEFINES   += WANT_ZYNADDSUBFX_UI

# Engine
PKGCONFIG += liblo

# RtAudio
DEFINES   += HAVE_GETTIMEOFDAY
DEFINES   += __RTAUDIO_DEBUG__ __RTMIDI_DEBUG__

# ALSA
DEFINES   += __LINUX_ALSA__ __LINUX_ALSASEQ__
PKGCONFIG += alsa

# JACK
DEFINES   += __UNIX_JACK__

# PulseAudio
DEFINES   += __LINUX_PULSE__
PKGCONFIG += libpulse-simple

# DISTRHO Plugin
DEFINES   += DISTRHO_PLUGIN_TARGET_VST

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
    ../../backend/engine/CarlaEnginePlugin.cpp \
    ../../backend/engine/CarlaEngineRtAudio.cpp

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
    ../../backend/plugin/Vst3Plugin.cpp \
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
    ../../backend/CarlaNative.h \
    ../../backend/CarlaNative.hpp \
    ../../backend/CarlaPlugin.hpp \
    ../../backend/CarlaStandalone.hpp

# engine
HEADERS += \
    ../../backend/engine/CarlaEngineInternal.hpp \
    ../../backend/engine/CarlaEngineOsc.hpp \
    ../../backend/engine/CarlaEngineThread.hpp \
    ../../backend/engine/distrho/DistrhoPluginInfo.h

# plugin
HEADERS += \
    ../../backend/plugin/CarlaPluginInternal.hpp \
    ../../backend/plugin/CarlaPluginThread.hpp

# includes
HEADERS += \
    ../../includes/CarlaDefines.hpp \
    ../../includes/CarlaMIDI.h \
    ../../includes/ladspa_rdf.hpp \
    ../../includes/lv2_rdf.hpp

# utils
HEADERS += \
    ../../utils/CarlaBackendUtils.hpp \
    ../../utils/CarlaJuceUtils.hpp \
    ../../utils/CarlaLadspaUtils.hpp \
    ../../utils/CarlaLibUtils.hpp \
    ../../utils/CarlaLv2Utils.hpp \
    ../../utils/CarlaOscUtils.hpp \
    ../../utils/CarlaShmUtils.hpp \
    ../../utils/CarlaStateUtils.hpp \
    ../../utils/CarlaVstUtils.hpp \
    ../../utils/CarlaUtils.hpp \
    ../../utils/CarlaMutex.hpp \
    ../../utils/CarlaString.hpp \
    ../../utils/CarlaThread.hpp \
    ../../utils/lv2_atom_queue.hpp \
    ../../utils/RtList.hpp

INCLUDEPATH = .. \
    ../../backend \
    ../../backend/engine \
    ../../backend/plugin \
    ../../backend/utils \
    ../../includes \
    ../../libs \
    ../../utils

# -----------------------------------------------------------

LIBS = -ldl \
    ../../backend/libcarla_native.a \
    ../../libs/dgl.a \
    ../../libs/lilv.a \
    ../../libs/rtmempool.a \
    ../../libs/widgets.a

QMAKE_CXXFLAGS *= -std=gnu++0x
