# QtCreator project file

QT = core

CONFIG    = debug link_pkgconfig qt warn_on
PKGCONFIG = liblo gtk+-3.0

TARGET   = carla-bridge-lv2-gtk3
TEMPLATE = app
VERSION  = 0.5.0

SOURCES = \
    ../carla_bridge_client.cpp \
    ../carla_bridge_osc.cpp \
    ../carla_bridge_toolkit.cpp \
    ../carla_bridge_toolkit-gtk.cpp \
    ../carla_bridge_ui-lv2.cpp

HEADERS = \
    ../carla_bridge.hpp \
    ../carla_bridge_client.hpp \
    ../carla_bridge_osc.hpp \
    ../carla_bridge_toolkit.hpp \
    ../../carla-includes/carla_defines.hpp \
    ../../carla-includes/carla_midi.h \
    ../../carla-includes/lv2_rdf.hpp \
    ../../carla-utils/carla_lib_utils.hpp \
    ../../carla-utils/carla_osc_utils.hpp \
    ../../carla-utils/carla_lv2_utils.hpp

INCLUDEPATH = .. \
    ../../carla-includes \
    ../../carla-utils

LIBS    = \
    ../../carla-lilv/carla_lilv.a \
    ../../carla-rtmempool/carla_rtmempool.a

DEFINES  = QTCREATOR_TEST
DEFINES += DEBUG
DEFINES += BUILD_BRIDGE BUILD_BRIDGE_UI BRIDGE_LV2 BRIDGE_GTK3 BRIDGE_LV2_GTK3

QMAKE_CXXFLAGS *= -std=c++0x
