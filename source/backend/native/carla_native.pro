# QtCreator project file

QT = core gui

CONFIG    = debug
CONFIG   += link_pkgconfig qt shared warn_on

DEFINES   = DEBUG
DEFINES  += HAVE_CPP11_SUPPORT
DEFINES  += QTCREATOR_TEST

# ZynAddSubFX
DEFINES  += WANT_ZYNADDSUBFX

PKGCONFIG = fftw3 mxml

TARGET   = carla_native
TEMPLATE = lib
VERSION  = 0.5.0

SOURCES  = \
    bypass.c \
    midi-split.c \
    midi-through.c \
    zynaddsubfx.cpp \
    zynaddsubfx-src.cpp

SOURCES += \
    distrho-3bandeq.cpp

HEADERS  = \
    ../carla_native.h \
    ../carla_native.hpp

HEADERS += \
    distrho/DistrhoPluginCarla.cpp

HEADERS += \
    ../../includes/carla_defines.hpp \
    ../../includes/carla_midi.h \
    ../../utils/carla_utils.hpp \
    ../../utils/carla_juce_utils.hpp

INCLUDEPATH = . .. \
    3bandeq distrho \
    ../../includes \
    ../../utils \
    ../../libs/distrho-plugin-toolkit

LIBS = -lGL -lpthread

QMAKE_CFLAGS   *= -std=c99
QMAKE_CXXFLAGS *= -std=c++0x
