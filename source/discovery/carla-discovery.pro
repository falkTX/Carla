# QtCreator project file

TARGET   = carla-discovery-qtcreator
TEMPLATE = app
VERSION  = 1.9

# -------------------------------------------------------

CONFIG  = debug
CONFIG += link_pkgconfig warn_on

# -------------------------------------------------------

DEFINES  = DEBUG REAL_BUILD
DEFINES += QTCREATOR_TEST

DEFINES += HAVE_FLUIDSYNTH
DEFINES += HAVE_LINUXSAMPLER
DEFINES += HAVE_X11
DEFINES += VESTIGE_HEADER
DEFINES += WANT_ZYNADDSUBFX
DEFINES += WANT_ZYNADDSUBFX_UI

# -------------------------------------------------------

PKGCONFIG  = fluidsynth
PKGCONFIG += linuxsampler

# -------------------------------------------------------

LIBS  = ../modules/juce_core.a
LIBS += ../modules/lilv.a
LIBS += -ldl

# -------------------------------------------------------

SOURCES = \
    carla-discovery.cpp

HEADERS  = \
    ../backend/CarlaBackend.hpp

HEADERS += \
    ../includes/CarlaDefines.hpp \
    ../includes/CarlaMIDI.h

HEADERS += \
    ../modules/ladspa_rdf.hpp \
    ../modules/lv2_rdf.hpp

HEADERS += \
    ../utils/CarlaUtils.hpp \
    ../utils/CarlaBackendUtils.hpp \
    ../utils/CarlaLibUtils.hpp \
    ../utils/CarlaMathUtils.hpp \
    ../utils/CarlaLadspaUtils.hpp \
    ../utils/CarlaDssiUtils.hpp \
    ../utils/CarlaLv2Utils.hpp \
    ../utils/CarlaVstUtils.hpp

INCLUDEPATH = \
    ../backend \
    ../includes \
    ../modules \
    ../utils

# -------------------------------------------------------

QMAKE_CXXFLAGS *= -std=c++11

# -------------------------------------------------------
