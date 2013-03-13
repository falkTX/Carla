# QtCreator project file

TARGET   = CarlaEngine
TEMPLATE = lib
VERSION  = 1.0

# -------------------------------------------------------

CONFIG     = debug
CONFIG    += link_pkgconfig shared warn_on

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
DEFINES   += WANT_AUDIOFILE
DEFINES   += WANT_ZYNADDSUBFX
PKGCONFIG  = gl

# Engine
PKGCONFIG += liblo QtCore

# RtAudio
DEFINES   += HAVE_GETTIMEOFDAY
DEFINES   += __RTAUDIO_DEBUG__ __RTMIDI_DEBUG__

# ALSA
DEFINES   += __LINUX_ALSA__ __LINUX_ALSASEQ__
PKGCONFIG += alsa

# JACK
DEFINES   += __UNIX_JACK__
PKGCONFIG += jack

# PulseAudio
DEFINES   += __LINUX_PULSE__
PKGCONFIG += libpulse-simple

# DISTRHO Plugin
DEFINES   += DISTRHO_PLUGIN_TARGET_VST

# -------------------------------------------------------

SOURCES  = \
    CarlaEngine.cpp \
    CarlaEngineOsc.cpp \
    CarlaEngineThread.cpp \
    CarlaEngineJack.cpp \
    CarlaEnginePlugin.cpp \
    CarlaEngineRtAudio.cpp

HEADERS  = \
    CarlaEngineInternal.hpp \
    CarlaEngineOsc.hpp \
    CarlaEngineThread.hpp

HEADERS += \
    ../CarlaBackend.hpp \
    ../CarlaEngine.hpp \
    ../CarlaPlugin.hpp

HEADERS += \
    ../../includes/CarlaDefines.hpp \
    ../../includes/CarlaMIDI.h \
    ../../utils/CarlaMutex.hpp \
    ../../utils/CarlaString.hpp \
    ../../utils/CarlaThread.hpp \
    ../../utils/CarlaUtils.hpp \
    ../../utils/CarlaBackendUtils.hpp \
    ../../utils/CarlaJuceUtils.hpp \
    ../../utils/CarlaOscUtils.hpp \
    ../../utils/CarlaStateUtils.hpp

HEADERS += \
    distrho/DistrhoPluginInfo.h

INCLUDEPATH = . .. plugin \
    ../../includes \
    ../../libs \
    ../../libs/distrho \
    ../../utils

# RtAudio/RtMidi
INCLUDEPATH += rtaudio-4.0.11 rtmidi-2.0.1
SOURCES     += rtaudio-4.0.11/RtAudio.cpp
SOURCES     += rtmidi-2.0.1/RtMidi.cpp

QMAKE_CXXFLAGS += -std=c++0x
