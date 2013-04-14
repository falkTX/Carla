# QtCreator project file

TARGET   = CarlaEngine
TEMPLATE = lib
VERSION  = 1.0

# -------------------------------------------------------

QT = core xml

CONFIG     = debug
CONFIG    += link_pkgconfig qt shared warn_on

DEFINES    = DEBUG
DEFINES   += HAVE_CPP11_SUPPORT
DEFINES   += QTCREATOR_TEST

# Shared
DEFINES   += WANT_NATIVE
DEFINES   += WANT_LADSPA
DEFINES   += WANT_DSSI
DEFINES   += WANT_LV2
DEFINES   += WANT_VST
DEFINES   += WANT_PLUGIN
DEFINES   += WANT_RTAUDIO
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

# -------------------------------------------------------

SOURCES  = \
    CarlaEngine.cpp \
    CarlaEngineOsc.cpp \
    CarlaEngineThread.cpp \
    CarlaEngineBridge.cpp \
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
    ../../utils/CarlaBridgeUtils.hpp \
    ../../utils/CarlaJuceUtils.hpp \
    ../../utils/CarlaOscUtils.hpp \
    ../../utils/CarlaStateUtils.hpp

HEADERS += \
    distrho/DistrhoPluginInfo.h

INCLUDEPATH = . .. plugin \
    ../../includes \
    ../../libs \
    ../../utils

# RtAudio/RtMidi
INCLUDEPATH += rtaudio-4.0.11 rtmidi-2.0.1
SOURCES     += rtaudio-4.0.11/RtAudio.cpp
SOURCES     += rtmidi-2.0.1/RtMidi.cpp

# Plugin
INCLUDEPATH += distrho
INCLUDEPATH += ../../libs/distrho
INCLUDEPATH += ../../includes/vst

QMAKE_CXXFLAGS += -std=c++0x
