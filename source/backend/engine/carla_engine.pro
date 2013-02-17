# QtCreator project file

CONFIG    = debug
CONFIG   += link_pkgconfig shared warn_on

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
    carla_engine.cpp \
    carla_engine_osc.cpp \
    carla_engine_thread.cpp \
    jack.cpp \
    plugin.cpp \
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
    ../../utils/carla_mutex.hpp \
    ../../utils/carla_string.hpp \
    ../../utils/carla_thread.hpp \
    ../../utils/carla_utils.hpp \
    ../../utils/carla_backend_utils.hpp \
    ../../utils/carla_juce_utils.hpp \
    ../../utils/carla_osc_utils.hpp \
    ../../utils/carla_state_utils.hpp

HEADERS += \
    plugin/DistrhoPluginInfo.h

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

# ---------------------------------------------------------------------------------------

PKGCONFIG += QtCore

# Fake includes
INCLUDEPATH += \
    /usr/include/qt4/ \
    /opt/kxstudio/include/

# System includes
QMAKE_CXXFLAGS += -isystem /usr/include/qt4/
QMAKE_CXXFLAGS += -isystem /opt/kxstudio/include/

WARN_FLAGS = \
    -ansi -pedantic -pedantic-errors -Wall -Wextra -Wformat=2 -Wunused-parameter -Wuninitialized \
    -Wcast-qual -Wconversion -Wsign-conversion -Wlogical-op -Waggregate-return  -Wno-vla \
    -fipa-pure-const -Wsuggest-attribute=const #pure,const,noreturn

QMAKE_CFLAGS   += $${WARN_FLAGS} -std=c99 -Wc++-compat -Wunsuffixed-float-constants -Wwrite-strings
# QMAKE_CXXFLAGS += $${WARN_FLAGS} -std=c++0x -fPIC
QMAKE_CXXFLAGS += $${WARN_FLAGS} -std=c++11 -Wzero-as-null-pointer-constant
