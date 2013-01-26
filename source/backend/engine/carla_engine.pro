# QtCreator project file

QT = core

CONFIG    = debug
CONFIG   += link_pkgconfig qt shared warn_on

DEFINES   = DEBUG
DEFINES  += QTCREATOR_TEST

# Misc
DEFINES  += __LINUX_ALSA__ __LINUX_ALSASEQ__
DEFINES  += __LINUX_PULSE__
DEFINES  += __UNIX_JACK__

# JACK
DEFINES  += CARLA_ENGINE_JACK

# RtAudio/RtMidi
DEFINES  += CARLA_ENGINE_RTAUDIO HAVE_GETTIMEOFDAY
DEFINES  += __RTAUDIO_DEBUG__ __RTMIDI_DEBUG__

# DISTRHO Plugin
DEFINES  += CARLA_ENGINE_PLUGIN
DEFINES  += DISTRHO_PLUGIN_TARGET_STANDALONE

# Misc
DEFINES  += WANT_LADSPA WANT_DSSI WANT_LV2 WANT_VST

PKGCONFIG = liblo jack alsa libpulse-simple

TARGET   = carla_engine
TEMPLATE = lib
VERSION  = 0.5.0

SOURCES  = \
    carla_engine.cpp \
#    carla_engine_osc.cpp \
#    carla_engine_thread.cpp \
     jack.cpp \
#      plugin.cpp \
     rtaudio.cpp

HEADERS  = \
    carla_engine_internal.hpp \
    carla_engine_osc.hpp \
    carla_engine_thread.hpp

HEADERS += \
    ../carla_backend.hpp \
    ../carla_engine.hpp \
    ../carla_plugin.hpp

HEADERS += \
    ../../includes/carla_defines.hpp \
    ../../includes/carla_midi.h \
    ../../utils/carla_utils.hpp \
    ../../utils/carla_backend_utils.hpp \
    ../../utils/carla_juce_utils.hpp

HEADERS += \
    plugin/DistrhoPluginInfo.h

INCLUDEPATH = . .. \
    ../../includes \
    ../../libs \
    ../../utils

# FIXME
INCLUDEPATH += \
    /opt/kxstudio/include

# RtAudio/RtMidi
INCLUDEPATH += rtaudio-4.0.11 rtmidi-2.0.1
SOURCES     += rtaudio-4.0.11/RtAudio.cpp
SOURCES     += rtmidi-2.0.1/RtMidi.cpp

# Plugin
INCLUDEPATH += ../../libs/distrho-plugin-toolkit

QMAKE_CXXFLAGS *= -std=c++0x
