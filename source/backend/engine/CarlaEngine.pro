# QtCreator project file

QT = core

CONFIG    = debug
CONFIG   += link_pkgconfig shared qt warn_on

DEFINES   = DEBUG
DEFINES  += QTCREATOR_TEST

# JACK
DEFINES  += CARLA_ENGINE_JACK
DEFINES  += __UNIX_JACK__

# RtAudio/RtMidi
DEFINES  += CARLA_ENGINE_RTAUDIO HAVE_GETTIMEOFDAY
DEFINES  += __RTAUDIO_DEBUG__ __RTMIDI_DEBUG__
DEFINES  += __LINUX_ALSA__ __LINUX_ALSASEQ__
DEFINES  += __LINUX_PULSE__

# DISTRHO Plugin
DEFINES  += CARLA_ENGINE_PLUGIN
DEFINES  += DISTRHO_PLUGIN_TARGET_STANDALONE

# Misc
DEFINES  += WANT_LADSPA WANT_DSSI WANT_LV2 WANT_VST
DEFINES  += WANT_JACK WANT_PLUGIN WANT_RTAUDIO

PKGCONFIG = liblo jack alsa libpulse-simple

TARGET   = carla_engine
TEMPLATE = lib
VERSION  = 0.5.0

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

INCLUDEPATH = . .. \
    ../../includes \
    ../../libs \
    ../../utils

# RtAudio/RtMidi
INCLUDEPATH += rtaudio-4.0.11 rtmidi-2.0.1
SOURCES     += rtaudio-4.0.11/RtAudio.cpp
SOURCES     += rtmidi-2.0.1/RtMidi.cpp

# Plugin
INCLUDEPATH += plugin ../../libs/distrho-plugin-toolkit

QMAKE_CXXFLAGS += -std=c++0x
