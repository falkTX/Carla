# QtCreator project file

TARGET   = CarlaPlugin
TEMPLATE = lib
VERSION  = 1.0

# -------------------------------------------------------

QT = core gui

CONFIG     = debug
CONFIG    += link_pkgconfig qt shared warn_on

DEFINES    = DEBUG
DEFINES   += HAVE_CPP11_SUPPORT
DEFINES   += QTCREATOR_TEST MOC_PARSING

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

# Audio file
DEFINES   += HAVE_FFMPEG
PKGCONFIG += sndfile libavcodec libavformat libavutil

# MIDI file
PKGCONFIG += smf

# DISTRHO
PKGCONFIG += gl

# ZynAddSubFX
DEFINES   += NTK_GUI
PKGCONFIG += fftw3 mxml zlib ntk ntk_images

# -------------------------------------------------------

SOURCES  = \
    bypass.c \
    lfo.c \
    midi-split.c \
    midi-through.c \
    midi-transpose.c \
    nekofilter.c

SOURCES += \
    audio-file.cpp \
    midi-file.cpp \
    midi-sequencer.cpp \
    sunvox-file.cpp \
    zynaddsubfx.cpp \
    zynaddsubfx-src.cpp \
    zynaddsubfx-ui.cpp

SOURCES += \
    distrho-3bandeq.cpp

HEADERS  = \
    audio-base.hpp \
    midi-base.hpp

HEADERS += \
    ../CarlaNative.h \
    ../CarlaNative.hpp

HEADERS += \
    distrho/DistrhoPluginCarla.cpp

HEADERS += \
    ../../utils/CarlaUtils.hpp \
    ../../utils/CarlaJuceUtils.hpp \
    ../../utils/CarlaLibUtils.hpp \
    ../../utils/CarlaOscUtils.hpp \
    ../../utils/CarlaStateUtils.hpp \
    ../../utils/CarlaMutex.hpp \
    ../../utils/CarlaString.hpp

INCLUDEPATH = . .. \
    3bandeq distrho \
    ../../includes \
    ../../libs/distrho \
    ../../utils \
    ../../widgets

QMAKE_CFLAGS   *= -std=c99
QMAKE_CXXFLAGS *= -std=c++0x
