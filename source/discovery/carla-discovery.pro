# QtCreator project file

QT = core

TARGET   = carla-discovery-qtcreator
TEMPLATE = app
VERSION  = 0.5.0

# -------------------------------------------------------

CONFIG     = debug
CONFIG    += link_pkgconfig qt shared warn_on

DEFINES    = DEBUG
DEFINES   += QTCREATOR_TEST

DEFINES   += WANT_LADSPA
DEFINES   += WANT_DSSI
DEFINES   += WANT_LV2
DEFINES   += WANT_VST
DEFINES   += WANT_FLUIDSYNTH
DEFINES   += WANT_LINUXSAMPLER

PKGCONFIG  = fluidsynth
PKGCONFIG += linuxsampler

SOURCES = \
    carla-discovery.cpp

# -------------------------------------------------------

HEADERS  = \
    ../backend/CarlaBackend.hpp

HEADERS += \
    ../includes/CarlaDefines.hpp \
    ../includes/CarlaMIDI.h \
    ../includes/ladspa_rdf.hpp \
    ../includes/lv2_rdf.hpp \
    ../utils/CarlaUtils.hpp \
    ../utils/CarlaBase64Utils.hpp \
    ../utils/CarlaJuceUtils.hpp \
    ../utils/CarlaLibUtils.hpp \
    ../utils/CarlaLadspaUtils.hpp \
    ../utils/CarlaLv2Utils.hpp \
    ../utils/CarlaVstUtils.hpp \
    ../utils/CarlaString.hpp

INCLUDEPATH = \
    ../backend \
    ../includes \
    ../utils

LIBS  = -ldl
LIBS += ../libs/lilv.a

QMAKE_CXXFLAGS *= -std=c++0x
