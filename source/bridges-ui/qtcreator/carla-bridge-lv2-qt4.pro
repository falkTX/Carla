# QtCreator project file

QT = core gui

TARGET   = carla-bridge-lv2-qt4
TEMPLATE = app

# -------------------------------------------------------

CONFIG    = debug
CONFIG   += link_pkgconfig qt warn_on

DEFINES   = DEBUG
DEFINES  += HAVE_CPP11_SUPPORT
DEFINES  += QTCREATOR_TEST

DEFINES  += BUILD_BRIDGE
DEFINES  += BUILD_BRIDGE_UI
DEFINES  += BRIDGE_LV2
DEFINES  += BRIDGE_QT4
DEFINES  += BRIDGE_LV2_QT4

PKGCONFIG = liblo

SOURCES = \
    ../CarlaBridgeClient.cpp \
    ../CarlaBridgeOsc.cpp \
    ../CarlaBridgeToolkit.cpp \
    ../CarlaBridgeToolkitQt.cpp \
    ../CarlaBridgeUI-LV2.cpp

HEADERS = \
    ../CarlaBridge.hpp \
    ../CarlaBridgeClient.hpp \
    ../CarlaBridgeOsc.hpp \
    ../CarlaBridgeToolkit.hpp \
    ../../includes/CarlaDefines.hpp \
    ../../includes/CarlaMIDI.h \
    ../../includes/lv2_rdf.hpp \
    ../../utils/CarlaJuceUtils.hpp \
    ../../utils/CarlaLibUtils.hpp \
    ../../utils/CarlaLv2Utils.hpp \
    ../../utils/CarlaOscUtils.hpp \
    ../../utils/CarlaUtils.hpp \
    ../../utils/CarlaString.hpp \
    ../../utils/RtList.hpp

INCLUDEPATH = .. \
    ../../includes \
    ../../libs \
    ../../theme \
    ../../utils

LIBS  = -ldl
LIBS += ../../libs/lilv.a
LIBS += ../../libs/rtmempool.a
LIBS += ../../libs/theme.a

QMAKE_CXXFLAGS *= -std=c++0x
