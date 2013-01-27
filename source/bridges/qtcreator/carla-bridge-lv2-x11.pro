# QtCreator project file

contains(QT_VERSION, ^5.*) {
QT = core widgets
} else {
QT = core gui
}

CONFIG    = debug link_pkgconfig qt warn_on
PKGCONFIG = liblo

TARGET   = carla-bridge-lv2-x11
TEMPLATE = app
VERSION  = 0.5.0

SOURCES = \
    ../carla_bridge_client.cpp \
    ../carla_bridge_osc.cpp \
    ../carla_bridge_toolkit.cpp \
    ../carla_bridge_toolkit-qt.cpp \
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

LIBS = \
    ../../carla-lilv/carla_lilv.a \
    ../../carla-rtmempool/carla_rtmempool.a

DEFINES  = QTCREATOR_TEST
DEFINES += DEBUG
DEFINES += BUILD_BRIDGE BUILD_BRIDGE_UI BRIDGE_LV2 BRIDGE_X11 BRIDGE_LV2_X11

QMAKE_CXXFLAGS *= -std=c++0x
