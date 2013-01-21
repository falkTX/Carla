# QtCreator project file

QT = core

win {
CONFIG    = release
} else {
CONFIG    = debug
}
CONFIG   += link_pkgconfig qt warn_on

DEFINES   = DEBUG
DEFINES  += WANT_LADSPA WANT_DSSI WANT_LV2 WANT_VST
DEFINES  += WANT_FLUIDSYNTH WANT_LINUXSAMPLER
PKGCONFIG = fluidsynth linuxsampler

TARGET   = carla-discovery-qtcreator
TEMPLATE = app
VERSION  = 0.5.0

SOURCES = \
    carla-discovery.cpp

HEADERS = \
    ../includes/carla_defines.hpp \
    ../includes/carla_midi.h \
    ../includes/ladspa_rdf.hpp \
    ../includes/lv2_rdf.hpp \
    ../backend/carla_backend.hpp \
    ../utils/carla_utils.hpp \
    ../utils/carla_juce_utils.hpp \
    ../utils/carla_lib_utils.hpp \
    ../utils/carla_ladspa_utils.hpp \
    ../utils/carla_lv2_utils.hpp \
    ../utils/carla_vst_utils.hpp

INCLUDEPATH = \
    ../backend \
    ../includes \
    ../utils

LIBS = \
    ../libs/lilv.a

unix {
LIBS += -ldl
}
mingw {
LIBS += -static -mwindows
}

QMAKE_CXXFLAGS *= -std=c++0x
